#include "../audear.h"

#if AEWINDOWS

#include <Windows.h>
#include <atlbase.h>

#include <initguid.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#include <Wmcodecdsp.h>
#include <dmo.h>
#pragma comment ( lib, "dmoguids.lib" )

#include <thread>

class AEWASAPIAudioPlayer : public AEAudioPlayer
{
public:
	AEWASAPIAudioPlayer ( IMMDevice * device, IAudioClient * audioClient, AUDCLNT_SHAREMODE shareMode )
		: device ( device ), audioClient ( audioClient ), audioRenderClient ( nullptr )
		, playerState ( kAEAPS_Stopped )
		, bufferingThread ( INVALID_HANDLE_VALUE ), renderingThread ( INVALID_HANDLE_VALUE )
	{
		HRESULT hr;

		QueryPerformanceFrequency ( &performanceFrequency );

		hr = audioClient->GetMixFormat ( &audioClientWaveFormat );

		if ( audioClientWaveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
		{
			WAVEFORMATEXTENSIBLE * extensible = ( WAVEFORMATEXTENSIBLE * ) audioClientWaveFormat;
			extensible->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
		}

		REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
		hr = audioClient->Initialize ( shareMode, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			hnsRequestedDuration, 0, audioClientWaveFormat, NULL );

		hEvent = CreateEvent ( nullptr, false, false, nullptr );
		hr = audioClient->SetEventHandle ( hEvent );
		hr = audioClient->GetBufferSize ( &audioClientBufferSize );

		bytesPerFrame = audioClientWaveFormat->nChannels * audioClientWaveFormat->wBitsPerSample / 8;

		hr = audioClient->GetService ( __uuidof( IAudioRenderClient ), ( void ** ) &audioRenderClient );
		hr = audioClient->GetService ( __uuidof ( ISimpleAudioVolume ), ( void ** ) &simpleAudioVolume );
	}

	~AEWASAPIAudioPlayer ()
	{
		if ( renderingThread != INVALID_HANDLE_VALUE )
		{
			TerminateThread ( renderingThread, 0 );
			WaitForSingleObject ( renderingThread, INFINITE );
			renderingThread = INVALID_HANDLE_VALUE;
		}
		if ( bufferingThread != INVALID_HANDLE_VALUE )
		{
			TerminateThread ( bufferingThread, 0 );
			WaitForSingleObject ( bufferingThread, INFINITE );
			bufferingThread = INVALID_HANDLE_VALUE;
		}

		CoTaskMemFree ( audioClientWaveFormat );
	}

public:
	virtual HRESULT __stdcall SetSourceStream ( AEAudioStream * stream )
	{
		HRESULT hr;

		if ( stream == nullptr )
			return E_INVALIDARG;

		if ( renderingThread != INVALID_HANDLE_VALUE )
		{
			TerminateThread ( renderingThread, 0 );
			WaitForSingleObject ( renderingThread, INFINITE );
			renderingThread = INVALID_HANDLE_VALUE;
		}
		if ( bufferingThread != INVALID_HANDLE_VALUE )
		{
			TerminateThread ( bufferingThread, 0 );
			WaitForSingleObject ( bufferingThread, INFINITE );
			bufferingThread = INVALID_HANDLE_VALUE;
		}

		if ( this->stream )
			this->stream.Release ();
		this->stream = stream;

		CComPtr<AEAudioDecoder> decoder;
		if ( FAILED ( hr = stream->GetSourceDecoder ( &decoder ) ) )
			return hr;

		WAVEFORMATEX * pwfx;
		if ( FAILED ( hr = decoder->GetWaveFormat ( &pwfx ) ) )
			return hr;

		byterate = pwfx->nAvgBytesPerSec;
		free ( pwfx );

		readPos = AETimeSpan ( 0 );

		bufferingThread = CreateThread ( nullptr, 0, [] ( LPVOID ptr ) -> DWORD
		{
			AEAudioStream * stream = ( AEAudioStream* ) ptr;
			while ( true )
			{
				if ( FAILED ( stream->Buffering () ) )
					break;
				Sleep ( 100 );
			}
			return 0;
		}, stream, 0, nullptr );
		renderingThread = CreateThread ( nullptr, 0, [] ( LPVOID ptr ) -> DWORD
		{
			AEWASAPIAudioPlayer * player = ( AEWASAPIAudioPlayer * ) ptr;
			while ( true )
			{
				player->OnBufferEnd ( ( void * ) TRUE );
				Sleep ( 1 );
			}
			return 0;
		}, this, 0, nullptr );

		return hr;
	}

public:
	virtual HRESULT __stdcall GetOutputFormat ( WAVEFORMATEX ** format )
	{
		//WAVEFORMATEX * temp = audioClientWaveFormat;
		//HRESULT hr = audioClient->GetMixFormat ( &temp );
		//if ( FAILED ( hr ) )
		//	return hr;
		//*format = ( WAVEFORMATEX* ) malloc ( temp->cbSize );
		//memcpy ( *format, temp, temp->cbSize );
		//CoTaskMemFree ( temp );
		*format = audioClientWaveFormat;
		return S_OK;
	}

public:
	virtual HRESULT __stdcall Play ()
	{
		HRESULT hr;
		if ( audioRenderClient == nullptr ) return E_FAIL;
		if ( SUCCEEDED ( hr = audioClient->Start () ) )
			playerState = kAEAPS_Playing;
		OnBufferEnd ( ( void * ) FALSE );
		return hr;
	}
	virtual HRESULT __stdcall Pause ()
	{
		HRESULT hr;
		if ( audioRenderClient == nullptr ) return E_FAIL;
		if ( SUCCEEDED ( hr = audioClient->Stop () ) )
			playerState = kAEAPS_Paused;
		return hr;
	}
	virtual HRESULT __stdcall Stop ()
	{
		HRESULT hr;
		if ( audioRenderClient == nullptr ) return E_FAIL;
		if ( FAILED ( hr = stream->Seek ( { 0 }, STREAM_SEEK_SET, nullptr ) ) )
			return hr;
		if ( SUCCEEDED ( hr = audioClient->Stop () ) )
			playerState = kAEAPS_Stopped;
		readedTime.QuadPart = 0;
		readPos = 0;
		SetPlayingPosition ( AETimeSpan ( 0 ) );
		return hr;
	}

public:
	virtual HRESULT __stdcall GetPlayingPosition ( AETimeSpan * pos )
	{
		if ( playerState == kAEAPS_Stopped )
		{
			*pos = 0;
			return S_OK;
		}
		else if ( playerState == kAEAPS_Paused )
		{
			return stream->GetCurrentPosition ( pos );
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter ( &currentTime );

		*pos = AETimeSpan ( readPos.GetTicks () + ( ( currentTime.QuadPart - readedTime.QuadPart ) * 10000000 / performanceFrequency.QuadPart ) );

		return S_OK;
	}
	virtual HRESULT __stdcall SetPlayingPosition ( const AETimeSpan & pos )
	{
		HRESULT hr;

		if ( playerState == kAEAPS_Playing )
			if ( FAILED ( hr = audioClient->Stop () ) )
				return hr;
		LARGE_INTEGER posInt;
		posInt.QuadPart = pos.GetBytes ( byterate );
		if ( FAILED ( hr = stream->Seek ( posInt, STREAM_SEEK_SET, nullptr ) ) )
			return hr;
		if ( playerState == kAEAPS_Playing )
			return audioClient->Start ();
		return S_OK;
	}
	virtual HRESULT __stdcall GetDuration ( AETimeSpan * duration )
	{
		if ( stream == nullptr )
			return E_FAIL;

		CComPtr<AEAudioDecoder> decoder;
		if ( FAILED ( stream->GetSourceDecoder ( &decoder ) ) )
			return E_FAIL;

		return decoder->GetDuration ( duration );
	}
	virtual HRESULT __stdcall GetPlayerState ( AEAudioPlayerState * state )
	{
		*state = playerState;
		return S_OK;
	}

public:
	virtual HRESULT __stdcall SetVolume ( float volume )
	{
		if ( audioRenderClient == nullptr )
			return E_FAIL;
		return simpleAudioVolume->SetMasterVolume ( volume, nullptr );
	}
	virtual HRESULT __stdcall GetVolume ( float * volume )
	{
		if ( audioRenderClient == nullptr )
			return E_FAIL;
		return simpleAudioVolume->GetMasterVolume ( volume );
	}

public:
	virtual void __stdcall OnBufferEnd ( void* pBufferContext )
	{
		bool wait = pBufferContext;

		if ( playerState != kAEAPS_Playing )
			return;
		else
		{

		}

		HRESULT hr;

		if ( wait )
			WaitForSingleObject ( hEvent, INFINITE );

		UINT paddingFrame;
		if ( FAILED ( audioClient->GetCurrentPadding ( &paddingFrame ) ) )
			return;

		UINT32 availableFrames = audioClientBufferSize - paddingFrame;
		int actualSize = availableFrames * bytesPerFrame;

		std::shared_ptr<BYTE []> readBuffer ( new BYTE [ actualSize ] );
		ULONG readed;
		if ( FAILED ( hr = stream->Read ( &readBuffer [ 0 ], actualSize, &readed ) ) )
		{
			playerState = kAEAPS_Stopped;
			readedTime.QuadPart = 0;
			readPos = 0;
			return;
		}

		readPos += AETimeSpan::FromSeconds ( readed / ( double ) byterate );

		BYTE * data;
		if ( FAILED ( hr = audioRenderClient->GetBuffer ( availableFrames, &data ) ) )
			return;

		memcpy ( data, &readBuffer [ 0 ], readed );

		audioRenderClient->ReleaseBuffer ( readed / bytesPerFrame, 0 );

		QueryPerformanceCounter ( &readedTime );
	}

private:
	CComPtr<IMMDevice> device;
	CComPtr<IAudioClient> audioClient;
	CComPtr<IAudioRenderClient> audioRenderClient;
	CComPtr<ISimpleAudioVolume> simpleAudioVolume;
	CComPtr<AEAudioStream> stream;

	HANDLE hEvent;
	UINT audioClientBufferSize;
	int bytesPerFrame;
	int byterate;

	WAVEFORMATEX * audioClientWaveFormat;

	AEAudioPlayerState playerState;

	AETimeSpan readPos;
	AETimeSpan playingPos;

	LARGE_INTEGER performanceFrequency;
	LARGE_INTEGER readedTime;
	AETimeSpan firstSampleTime;

	HANDLE bufferingThread, renderingThread;
};

HRESULT AE_CreateWASAPIAudioPlayer ( IMMDevice * device, AUDCLNT_SHAREMODE shareMode, AEAudioPlayer ** audioPlayer )
{
	if ( device == nullptr || audioPlayer == nullptr )
		return E_INVALIDARG;

	HRESULT hr;
	CComPtr<IAudioClient> audioClient;
	if ( FAILED ( hr = device->Activate ( __uuidof ( IAudioClient ), CLSCTX_ALL,
		NULL, ( void** ) &audioClient ) ) )
		return hr;

	*audioPlayer = new AEWASAPIAudioPlayer ( device, audioClient, shareMode );

	return S_OK;
}

#endif