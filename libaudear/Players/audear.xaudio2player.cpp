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
	virtual AEERROR setSourceStream ( AEBaseAudioStream * stream )
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
	virtual AEERROR play ()
	{
		HRESULT hr;
		if ( sourceVoice == nullptr ) return AEERROR_FAIL;
		if ( SUCCEEDED ( hr = sourceVoice->Start () ) )
		{
			playerState = kAEPLAYERSTATE_PLAYING;
			OnBufferEnd ( nullptr );
		}
		return SUCCEEDED ( hr ) ? AEERROR_SUCCESS : AEERROR_FAIL;
	}
	virtual AEERROR pause ()
	{
		HRESULT hr;
		if ( sourceVoice == nullptr ) return AEERROR_FAIL;
		if ( SUCCEEDED ( hr = sourceVoice->Stop () ) )
		{
			playerState = kAEPLAYERSTATE_PAUSED;
		}
		return SUCCEEDED ( hr ) ? AEERROR_SUCCESS : AEERROR_FAIL;
	}
	virtual AEERROR stop ()
	{
		HRESULT hr;
		AEERROR et;
		if ( sourceVoice == nullptr ) return AEERROR_FAIL;
		if ( FAILED ( et = stream->seek ( kAESTREAMSEEK_SET, 0, nullptr ) ) )
			return et;
		if ( SUCCEEDED ( hr = sourceVoice->Stop () ) )
		{
			playerState = kAEPLAYERSTATE_STOPPED;
			readedTime.QuadPart = 0;
			readPos = 0;
			setPosition ( AETimeSpan ( 0 ) );
		}
		return SUCCEEDED ( hr ) ? AEERROR_SUCCESS : AEERROR_FAIL;
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
			AEERROR et;
			int64_t streamPos;
			if ( FAILED ( et = stream->getPosition ( &streamPos ) ) )
				return et;
			*pos = AETimeSpan::fromByteCount ( streamPos, byterate );
			return AEERROR_SUCCESS;
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter ( &currentTime );

		*pos = AETimeSpan ( readPos.getTicks () + ( ( currentTime.QuadPart - readedTime.QuadPart ) * 10000000 / performanceFrequency.QuadPart ) );

		return AEERROR_SUCCESS;
	}
	virtual AEERROR setPosition ( AETimeSpan pos )
	{
		HRESULT hr;
		AEERROR et;
		if ( playerState == kAEPLAYERSTATE_PLAYING )
			if ( FAILED ( hr = sourceVoice->Stop () ) )
				return AEERROR_FAIL;
		if ( FAILED ( et = stream->seek ( kAESTREAMSEEK_SET, pos.getByteCount ( byterate ), nullptr ) ) )
			return et;
		if ( playerState == kAEPLAYERSTATE_PLAYING )
			return FAILED ( sourceVoice->Start () ) ? AEERROR_FAIL : AEERROR_SUCCESS;
		return AEERROR_SUCCESS;
	}
	virtual AEERROR getDuration ( AETimeSpan * duration )
	{
		if ( stream == nullptr )
			return AEERROR_FAIL;

		AEAutoPtr<AEBaseAudioDecoder> decoder;
		if ( FAILED ( stream->getBaseDecoder ( &decoder ) ) )
			return AEERROR_FAIL;

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
		if ( sourceVoice == nullptr )
			return AEERROR_FAIL;
		return SUCCEEDED ( sourceVoice->SetVolume ( volume ) ) ? AEERROR_SUCCESS : AEERROR_FAIL;
	}
	virtual AEERROR getVolume ( float * volume )
	{
		if ( sourceVoice == nullptr )
			return AEERROR_FAIL;
		sourceVoice->GetVolume ( volume );
		return AEERROR_SUCCESS;
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

AEERROR AE_createXAudio2Player ( IXAudio2 * xaudio2, AEBaseAudioPlayer ** audioPlayer )
{
	if ( xaudio2 == nullptr || audioPlayer == nullptr )
		return AEERROR_INVALID_ARGUMENT;

	*audioPlayer = new AEXAudioAudioPlayer ( xaudio2 );

	return AEERROR_SUCCESS;
}

#endif