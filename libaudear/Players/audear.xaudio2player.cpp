#include "../audear.h"

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP

#include <xaudio2.h>
#pragma comment ( lib, "XAudio2.lib" )

class AEXAudioAudioPlayer : public AEBaseAudioPlayer, public IXAudio2VoiceCallback
{
public:
	AEXAudioAudioPlayer ( IXAudio2 * xaudio2 )
		: xaudio2 ( xaudio2 ), sourceVoice ( nullptr )
		, playerState ( kAEPLAYERSTATE_STOPPED )
	{
		QueryPerformanceFrequency ( &performanceFrequency );
	}

	~AEXAudioAudioPlayer ()
	{
		if ( sourceVoice )
			sourceVoice->DestroyVoice ();
	}

public:
	virtual error_t setSourceStream ( AEBaseAudioStream * stream )
	{
		HRESULT hr;

		if ( stream == nullptr )
			return E_INVALIDARG;

		if ( sourceVoice )
		{
			sourceVoice->DestroyVoice ();
			sourceVoice = nullptr;
		}

		this->stream.release ();
		this->stream = stream;

		AEAutoPtr<AEBaseAudioDecoder> decoder;
		if ( FAILED ( hr = stream->getBaseDecoder ( &decoder ) ) )
			return hr;

		AEWaveFormat waveFormat;
		if ( FAILED ( hr = decoder->getWaveFormat ( &waveFormat ) ) )
			return hr;
		WAVEFORMATEX * pwfx = waveFormat.getWaveFormatEx ();
		hr = xaudio2->CreateSourceVoice ( &sourceVoice, pwfx, 0,
			XAUDIO2_DEFAULT_FREQ_RATIO, this );
		CoTaskMemFree ( pwfx );

		byterate = waveFormat.getByteRate ();
		readingSize = byterate / 100;

		readPos = AETimeSpan ( 0 );

		return hr;
	}

public:
	virtual error_t play ()
	{
		HRESULT hr;
		if ( sourceVoice == nullptr ) return AE_ERROR_FAIL;
		if ( SUCCEEDED ( hr = sourceVoice->Start () ) )
		{
			playerState = kAEPLAYERSTATE_PLAYING;
			OnBufferEnd ( nullptr );
		}
		return SUCCEEDED ( hr ) ? AE_ERROR_SUCCESS : AE_ERROR_FAIL;
	}
	virtual error_t pause ()
	{
		HRESULT hr;
		if ( sourceVoice == nullptr ) return AE_ERROR_FAIL;
		if ( SUCCEEDED ( hr = sourceVoice->Stop () ) )
		{
			playerState = kAEPLAYERSTATE_PAUSED;
		}
		return SUCCEEDED ( hr ) ? AE_ERROR_SUCCESS : AE_ERROR_FAIL;
	}
	virtual error_t stop ()
	{
		HRESULT hr;
		error_t et;
		if ( sourceVoice == nullptr ) return AE_ERROR_FAIL;
		if ( FAILED ( et = stream->seek ( kAESTREAMSEEK_SET, 0, nullptr ) ) )
			return et;
		if ( SUCCEEDED ( hr = sourceVoice->Stop () ) )
		{
			playerState = kAEPLAYERSTATE_STOPPED;
			readedTime.QuadPart = 0;
			readPos = 0;
			setPosition ( AETimeSpan ( 0 ) );
		}
		return SUCCEEDED ( hr ) ? AE_ERROR_SUCCESS : AE_ERROR_FAIL;
	}

public:
	virtual error_t getPosition ( AETimeSpan * pos )
	{
		if ( playerState == kAEPLAYERSTATE_STOPPED )
		{
			*pos = 0;
			return AE_ERROR_SUCCESS;
		}
		else if ( playerState == kAEPLAYERSTATE_PAUSED )
		{
			error_t et;
			int64_t streamPos;
			if ( FAILED ( et = stream->getPosition ( &streamPos ) ) )
				return et;
			*pos = AETimeSpan::fromByteCount ( streamPos, byterate );
			return AE_ERROR_SUCCESS;
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter ( &currentTime );

		*pos = AETimeSpan ( readPos.getTicks () + ( ( currentTime.QuadPart - readedTime.QuadPart ) * 10000000 / performanceFrequency.QuadPart ) );

		return AE_ERROR_SUCCESS;
	}
	virtual error_t setPosition ( AETimeSpan pos )
	{
		HRESULT hr;
		error_t et;
		if ( playerState == kAEPLAYERSTATE_PLAYING )
			if ( FAILED ( hr = sourceVoice->Stop () ) )
				return AE_ERROR_FAIL;
		if ( FAILED ( et = stream->seek ( kAESTREAMSEEK_SET, pos.getByteCount ( byterate ), nullptr ) ) )
			return et;
		if ( playerState == kAEPLAYERSTATE_PLAYING )
			return FAILED ( sourceVoice->Start () ) ? AE_ERROR_FAIL : AE_ERROR_SUCCESS;
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getDuration ( AETimeSpan * duration )
	{
		if ( stream == nullptr )
			return AE_ERROR_FAIL;

		AEAutoPtr<AEBaseAudioDecoder> decoder;
		if ( FAILED ( stream->getBaseDecoder ( &decoder ) ) )
			return AE_ERROR_FAIL;

		return decoder->getDuration ( duration );
	}
	virtual error_t getState ( AEPLAYERSTATE * state )
	{
		*state = playerState;
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t setVolume ( float volume )
	{
		if ( sourceVoice == nullptr )
			return AE_ERROR_FAIL;
		return SUCCEEDED ( sourceVoice->SetVolume ( volume ) ) ? AE_ERROR_SUCCESS : AE_ERROR_FAIL;
	}
	virtual error_t getVolume ( float * volume )
	{
		if ( sourceVoice == nullptr )
			return AE_ERROR_FAIL;
		sourceVoice->GetVolume ( volume );
		return AE_ERROR_SUCCESS;
	}

public:
	virtual void __stdcall OnVoiceProcessingPassStart ( UINT32 BytesRequired ) { }
	virtual void __stdcall OnVoiceProcessingPassEnd () { }
	virtual void __stdcall OnStreamEnd () { }
	virtual void __stdcall OnBufferStart ( void* pBufferContext ) { }
	virtual void __stdcall OnBufferEnd ( void* pBufferContext )
	{
		if ( playerState != kAEPLAYERSTATE_PLAYING )
			return;

		HRESULT hr;

		BYTE readBuffer [ 524288 ];
		int64_t readed;
		if ( FAILED ( hr = stream->read ( &readBuffer [ 0 ], readingSize, &readed ) ) )
		{
			//if ( hr == E_EOF )
			{
				XAUDIO2_BUFFER xBuffer = {};
				xBuffer.Flags = XAUDIO2_END_OF_STREAM;
				sourceVoice->SubmitSourceBuffer ( &xBuffer );

				playerState = kAEPLAYERSTATE_STOPPED;
				readedTime.QuadPart = 0;
				readPos = 0;

				return;
			}
		}

		//readPos += AETimeSpan::fromByteCount ( readed, byterate );
		int64_t pos;
		stream->getPosition ( &pos );
		readPos = AETimeSpan::fromByteCount ( pos, byterate );

		XAUDIO2_BUFFER xBuffer = {};
		xBuffer.pAudioData = readBuffer;
		xBuffer.AudioBytes = ( UINT32 ) readed;
		if ( FAILED ( hr = sourceVoice->SubmitSourceBuffer ( &xBuffer ) ) )
		{
			return;
		}

		QueryPerformanceCounter ( &readedTime );
	}
	virtual void __stdcall OnLoopEnd ( void* pBufferContext ) { }
	virtual void __stdcall OnVoiceError ( void* pBufferContext, HRESULT Error ) { }

private:
	CComPtr<IXAudio2> xaudio2;
	IXAudio2SourceVoice * sourceVoice;
	AEAutoPtr<AEBaseAudioStream> stream;
	int readingSize;
	int byterate;

	AEPLAYERSTATE playerState;

	AETimeSpan readPos;
	AETimeSpan playingPos;

	LARGE_INTEGER performanceFrequency;
	LARGE_INTEGER readedTime;
	AETimeSpan firstSampleTime;
};

error_t AE_createXAudio2Player ( IXAudio2 * xaudio2, AEBaseAudioPlayer ** audioPlayer )
{
	if ( xaudio2 == nullptr || audioPlayer == nullptr )
		return AE_ERROR_INVALID_ARGUMENT;

	*audioPlayer = new AEXAudioAudioPlayer ( xaudio2 );

	return AE_ERROR_SUCCESS;
}

#endif