#include "../audear.h"

#if AEWINDOWS

#include <Windows.h>
#include <atlbase.h>
#include <xaudio2.h>

#include <thread>

class AEXAudioAudioPlayer : public AEAudioPlayer, public IXAudio2VoiceCallback
{
public:
	AEXAudioAudioPlayer ( IXAudio2 * xaudio2 )
		: xaudio2 ( xaudio2 ), sourceVoice ( nullptr ), playerState ( kAEAPS_Stopped ), bufferingThread ( INVALID_HANDLE_VALUE )
	{
		QueryPerformanceFrequency ( &performanceFrequency );
	}

	~AEXAudioAudioPlayer ()
	{
		if ( bufferingThread != INVALID_HANDLE_VALUE )
		{
			TerminateThread ( bufferingThread, 0 );
			WaitForSingleObject ( bufferingThread, INFINITE );
			bufferingThread = INVALID_HANDLE_VALUE;
		}

		if ( sourceVoice )
			sourceVoice->DestroyVoice ();
	}

public:
	virtual HRESULT __stdcall SetSourceStream ( AEAudioStream * stream )
	{
		HRESULT hr;

		if ( stream == nullptr )
			return E_INVALIDARG;

		if ( bufferingThread != INVALID_HANDLE_VALUE )
		{
			TerminateThread ( bufferingThread, 0 );
			WaitForSingleObject ( bufferingThread, INFINITE );
			bufferingThread = INVALID_HANDLE_VALUE;
		}

		if ( sourceVoice )
		{
			sourceVoice->DestroyVoice ();
			sourceVoice = nullptr;
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
		hr = xaudio2->CreateSourceVoice ( &sourceVoice, pwfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, this );
		readingSize = pwfx->nAvgBytesPerSec / 100;
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

		return hr;
	}

public:
	virtual HRESULT __stdcall GetOutputFormat ( WAVEFORMATEX ** format )
	{
		*format = nullptr;
		return E_NOTIMPL;
	}

public:
	virtual HRESULT __stdcall Play ()
	{
		HRESULT hr;
		if ( sourceVoice == nullptr ) return E_FAIL;
		if ( SUCCEEDED ( hr = sourceVoice->Start () ) )
			playerState = kAEAPS_Playing;
		OnBufferEnd ( nullptr );
		return hr;
	}
	virtual HRESULT __stdcall Pause ()
	{
		HRESULT hr;
		if ( sourceVoice == nullptr ) return E_FAIL;
		if ( SUCCEEDED ( hr = sourceVoice->Stop () ) )
			playerState = kAEAPS_Paused;
		return hr;
	}
	virtual HRESULT __stdcall Stop ()
	{
		HRESULT hr;
		if ( sourceVoice == nullptr ) return E_FAIL;
		if ( FAILED ( hr = stream->Seek ( { 0 }, STREAM_SEEK_SET, nullptr ) ) )
			return hr;
		if ( SUCCEEDED ( hr = sourceVoice->Stop () ) )
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
			if ( FAILED ( hr = sourceVoice->Stop () ) )
				return hr;
		LARGE_INTEGER posInt;
		posInt.QuadPart = pos.GetBytes ( byterate );
		if ( FAILED ( hr = stream->Seek ( posInt, STREAM_SEEK_SET, nullptr ) ) )
			return hr;
		if ( playerState == kAEAPS_Playing )
			return sourceVoice->Start ();
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
		if ( sourceVoice == nullptr )
			return E_FAIL;
		return sourceVoice->SetVolume ( volume );
	}
	virtual HRESULT __stdcall GetVolume ( float * volume )
	{
		if ( sourceVoice == nullptr )
			return E_FAIL;
		sourceVoice->GetVolume ( volume );
		return S_OK;
	}

public:
	virtual void __stdcall OnVoiceProcessingPassStart ( UINT32 BytesRequired ) { }
	virtual void __stdcall OnVoiceProcessingPassEnd () { }
	virtual void __stdcall OnStreamEnd () { }
	virtual void __stdcall OnBufferStart ( void* pBufferContext ) { }
	virtual void __stdcall OnBufferEnd ( void* pBufferContext )
	{
		if ( playerState != kAEAPS_Playing )
			return;

		HRESULT hr;

		std::shared_ptr<BYTE[]> readBuffer ( new BYTE [ readingSize ] );
		ULONG readed;
		if ( FAILED ( hr = stream->Read ( &readBuffer [ 0 ], readingSize, &readed ) ) )
		{
			//if ( hr == E_EOF )
			{
				XAUDIO2_BUFFER xBuffer = {};
				xBuffer.Flags = XAUDIO2_END_OF_STREAM;
				sourceVoice->SubmitSourceBuffer ( &xBuffer );

				playerState = kAEAPS_Stopped;
				readedTime.QuadPart = 0;
				readPos = 0;
				return;
			}
		}

		readPos += AETimeSpan::FromByteCount ( readed, byterate );

		XAUDIO2_BUFFER xBuffer = {};
		xBuffer.pAudioData = &readBuffer [ 0 ];
		xBuffer.AudioBytes = readed;
		if ( FAILED ( hr = sourceVoice->SubmitSourceBuffer ( &xBuffer ) ) )
			return;

		QueryPerformanceCounter ( &readedTime );
	}
	virtual void __stdcall OnLoopEnd ( void* pBufferContext ) { }
	virtual void __stdcall OnVoiceError ( void* pBufferContext, HRESULT Error ) { }

private:
	CComPtr<IXAudio2> xaudio2;
	IXAudio2SourceVoice * sourceVoice;
	CComPtr<AEAudioStream> stream;
	int readingSize;
	int byterate;

	AEAudioPlayerState playerState;

	AETimeSpan readPos;
	AETimeSpan playingPos;

	LARGE_INTEGER performanceFrequency;
	LARGE_INTEGER readedTime;
	AETimeSpan firstSampleTime;

	HANDLE bufferingThread;
};

HRESULT AE_CreateXAudio2AudioPlayer ( IXAudio2 * xaudio2, AEAudioPlayer ** audioPlayer )
{
	if ( xaudio2 == nullptr || audioPlayer == nullptr )
		return E_INVALIDARG;

	*audioPlayer = new AEXAudioAudioPlayer ( xaudio2 );

	return S_OK;
}

#endif