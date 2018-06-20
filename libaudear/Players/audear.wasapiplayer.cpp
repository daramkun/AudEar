#include "../audear.h"
#include "../Streams/Win32/audear.dmoresamplerstream.h"

#include <initguid.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <Wmcodecdsp.h>
#include <mediaobj.h>

#pragma comment ( lib, "dmoguids.lib" )
#pragma comment ( lib, "wmcodecdspuuid.lib" )

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

class AEWASAPIAudioPlayer : public AEBaseAudioPlayer
{
public:
	AEWASAPIAudioPlayer ( IMMDevice * device, IAudioClient * audioClient, AUDCLNT_SHAREMODE shareMode )
		: device ( device ), audioClient ( audioClient ), audioRenderClient ( nullptr ), shareMode ( shareMode )
		, playerState ( kAEPLAYERSTATE_STOPPED ), renderingThread ( INVALID_HANDLE_VALUE )
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

		bytesPerFrame = audioClientWaveFormat->nBlockAlign;

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

		CoTaskMemFree ( audioClientWaveFormat );
	}

public:
	virtual AEERROR setSourceStream ( AEBaseAudioStream * stream )
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

		this->stream.release ();
		this->stream = stream;

		AEAutoPtr<AEBaseAudioDecoder> decoder;
		if ( FAILED ( hr = stream->getBaseDecoder ( &decoder ) ) )
			return hr;

		if ( FAILED ( hr = decoder->getWaveFormat ( &waveFormat ) ) )
			return hr;

		if ( !( AEWaveFormat ( audioClientWaveFormat ) == waveFormat ) )
		{
			WAVEFORMATEX * outputFormat;
			audioClient->GetMixFormat ( &outputFormat );
			if ( outputFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
			{
				WAVEFORMATEXTENSIBLE * extensible = ( WAVEFORMATEXTENSIBLE * ) outputFormat;
				extensible->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			}
			this->stream.release ();
			if ( FAILED ( AE_internal_createDMOResamplerStream ( stream, outputFormat, &this->stream ) ) )
				return AEERROR_FAIL;
		}

		readPos = AETimeSpan ( 0 );
		readedTime.QuadPart = 0;

		return hr;
	}

public:
	virtual AEERROR play ()
	{
		HRESULT hr;
		if ( audioRenderClient == nullptr ) return E_FAIL;
		if ( SUCCEEDED ( hr = audioClient->Start () ) )
			playerState = kAEPLAYERSTATE_PLAYING;
		//OnBufferEnd ( ( void * ) FALSE );
		QueryPerformanceCounter ( &readedTime );
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
	virtual AEERROR pause ()
	{
		HRESULT hr;
		if ( audioRenderClient == nullptr ) return E_FAIL;
		if ( SUCCEEDED ( hr = audioClient->Stop () ) )
			playerState = kAEPLAYERSTATE_PAUSED;
		if ( renderingThread != INVALID_HANDLE_VALUE )
		{
			TerminateThread ( renderingThread, 0 );
			WaitForSingleObject ( renderingThread, INFINITE );
			renderingThread = INVALID_HANDLE_VALUE;
		}
		return hr;
	}
	virtual AEERROR stop ()
	{
		HRESULT hr;
		if ( audioRenderClient == nullptr ) return E_FAIL;
		if ( FAILED ( hr = stream->seek ( kAESTREAMSEEK_SET, 0, nullptr ) ) )
			return hr;
		if ( SUCCEEDED ( hr = audioClient->Stop () ) )
			playerState = kAEPLAYERSTATE_STOPPED;
		readedTime.QuadPart = 0;
		readPos = 0;
		setPosition ( AETimeSpan ( 0 ) );
		if ( renderingThread != INVALID_HANDLE_VALUE )
		{
			TerminateThread ( renderingThread, 0 );
			WaitForSingleObject ( renderingThread, INFINITE );
			renderingThread = INVALID_HANDLE_VALUE;
		}
		return hr;
	}

public:
	virtual AEERROR getPosition ( AETimeSpan * pos )
	{
		if ( playerState == kAEPLAYERSTATE_STOPPED )
		{
			*pos = 0;
			return AEERROR_SUCCESS;
		}
		else if ( playerState == kAEPLAYERSTATE_PAUSED )
		{
			int64_t temp;
			AEERROR et = stream->getPosition ( &temp );
			if ( FAILED ( et ) ) return et;
			*pos = AETimeSpan::fromByteCount ( temp, audioClientWaveFormat->nAvgBytesPerSec );
			return AEERROR_SUCCESS;
 		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter ( &currentTime );

		*pos = AETimeSpan ( readPos.getTicks () + ( ( currentTime.QuadPart - readedTime.QuadPart ) * 10000000 / performanceFrequency.QuadPart ) );

		return AEERROR_SUCCESS;
	}
	virtual AEERROR setPosition ( AETimeSpan time )
	{
		HRESULT hr;

		if ( playerState == kAEPLAYERSTATE_PLAYING )
			if ( FAILED ( hr = audioClient->Stop () ) )
				return hr;
		if ( FAILED ( hr = stream->seek ( kAESTREAMSEEK_SET, time.getByteCount ( audioClientWaveFormat->nAvgBytesPerSec ), nullptr ) ) )
			return hr;
		if ( playerState == kAEPLAYERSTATE_PLAYING )
			return audioClient->Start ();
		return AEERROR_SUCCESS;
	}
	virtual AEERROR getDuration ( AETimeSpan * duration )
	{
		if ( stream == nullptr )
			return E_FAIL;

		AEAutoPtr<AEBaseAudioDecoder> decoder;
		if ( FAILED ( stream->getBaseDecoder ( &decoder ) ) )
			return E_FAIL;

		return decoder->getDuration ( duration );
	}
	virtual AEERROR getState ( AEPLAYERSTATE * state )
	{
		*state = playerState;
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR setVolume ( float volume )
	{
		if ( audioRenderClient == nullptr )
			return E_FAIL;
		return simpleAudioVolume->SetMasterVolume ( volume, nullptr );
	}
	virtual AEERROR getVolume ( float * volume )
	{
		if ( audioRenderClient == nullptr )
			return E_FAIL;
		return simpleAudioVolume->GetMasterVolume ( volume );
	}

public:
	void OnBufferEnd ( bool wait )
	{
		if ( playerState != kAEPLAYERSTATE_PLAYING )
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
		int64_t readed;
		if ( FAILED ( hr = stream->read ( &readBuffer [ 0 ], actualSize, &readed ) ) )
		{
			playerState = kAEPLAYERSTATE_STOPPED;
			readedTime.QuadPart = 0;
			readPos = 0;
			return;
		}

		//readPos += AETimeSpan::fromByteCount ( readed, waveFormat.getByteRate () );
		int64_t pos;
		stream->getPosition ( &pos );
		readPos = AETimeSpan::fromByteCount ( pos, audioClientWaveFormat->nAvgBytesPerSec );

		BYTE * data;
		if ( FAILED ( hr = audioRenderClient->GetBuffer ( availableFrames, &data ) ) )
			return;

		memcpy ( data, &readBuffer [ 0 ], readed );

		audioRenderClient->ReleaseBuffer ( ( UINT ) readed / bytesPerFrame, 0 );

		QueryPerformanceCounter ( &readedTime );
	}

private:
	CComPtr<IMMDevice> device;
	CComPtr<IAudioClient> audioClient;
	CComPtr<IAudioRenderClient> audioRenderClient;
	CComPtr<ISimpleAudioVolume> simpleAudioVolume;
	AEAutoPtr<AEBaseAudioStream> stream;

	HANDLE hEvent;
	UINT audioClientBufferSize;
	int bytesPerFrame;

	WAVEFORMATEX * audioClientWaveFormat;
	AEWaveFormat waveFormat;

	AEPLAYERSTATE playerState;

	AETimeSpan readPos;
	AETimeSpan playingPos;

	LARGE_INTEGER performanceFrequency;
	LARGE_INTEGER readedTime;
	AETimeSpan firstSampleTime;

	HANDLE renderingThread;

	AUDCLNT_SHAREMODE shareMode;
};

EXTC AEEXP AEERROR AE_createWASAPIPlayer ( IMMDevice * mmDevice, AUDCLNT_SHAREMODE shareMode, AEBaseAudioPlayer ** player )
{
	if ( mmDevice == nullptr || player == nullptr ) return AEERROR_INVALID_ARGUMENT;

	HRESULT hr;
	CComPtr<IAudioClient> audioClient;
	if ( FAILED ( hr = mmDevice->Activate ( __uuidof ( IAudioClient ), CLSCTX_ALL,
		NULL, ( void** ) &audioClient ) ) )
		return AEERROR_FAIL;

	*player = new AEWASAPIAudioPlayer ( mmDevice, audioClient, shareMode );
	return AEERROR_SUCCESS;
}