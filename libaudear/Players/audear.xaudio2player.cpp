#include "../audear.h"
#include "../InnerUtilities/HighResolutionTimer.hpp"

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP

#include <atlbase.h>
#include <xaudio2.h>
#pragma comment ( lib, "XAudio2.lib" )

class __XAudio2AudioPlayer : public IXAudio2VoiceCallback
{
public:
	__XAudio2AudioPlayer ( XAUDIO2_PROCESSOR processor )
		:  _state ( AEPS_STOPPED ), _sourceStream ( nullptr ), _sourceVoice ( nullptr )
		, _readedTime ( 0 ), _sampleTime ( { 0 } )
	{
		_performanceFrequency = __HRT_GetFrequency ();

		XAudio2Create ( &_xaudio2, 0, processor );
		_xaudio2->CreateMasteringVoice ( &_masteringVoice );

		_xaudio2->StartEngine ();
	}
	~__XAudio2AudioPlayer ()
	{
		if ( _sourceVoice )
			_sourceVoice->DestroyVoice ();
		_sourceVoice = nullptr;
		_masteringVoice->DestroyVoice ();
		_masteringVoice = nullptr;

		_xaudio2->StopEngine ();
	}

public:

	error_t play ()
	{
		if ( _state == AEPS_PLAYING ) return AEERROR_NOERROR;
		if ( _sourceVoice == nullptr ) return AEERROR_INVALID_CALL;

		OnBufferEnd ( ( void * ) 0x12345678 );
		if ( FAILED ( _sourceVoice->Start () ) )
			return AEERROR_FAIL;

		_state = AEPS_PLAYING;

		return AEERROR_NOERROR;
	}
	error_t pause ()
	{
		if ( _state == AEPS_PAUSED ) return AEERROR_NOERROR;
		if ( _sourceVoice == nullptr ) return AEERROR_INVALID_CALL;

		OnBufferEnd ( ( void * ) 0x12345678 );
		if ( FAILED ( _sourceVoice->Stop () ) )
			return AEERROR_FAIL;

		_state = AEPS_PAUSED;

		return AEERROR_NOERROR;
	}
	error_t stop ()
	{
		if ( _state == AEPS_STOPPED ) return AEERROR_NOERROR;
		if ( _sourceVoice == nullptr ) return AEERROR_INVALID_CALL;

		if ( FAILED ( _sourceVoice->Stop () ) )
			return AEERROR_FAIL;

		_sourceStream->seek ( _sourceStream->object, 0, AESO_BEGIN );

		_state = AEPS_STOPPED;

		return AEERROR_NOERROR;
	}

public:
	error_t state ( AEPLAYERSTATE * state )
	{
		*state = _state;
		return AEERROR_NOERROR;
	}

public:
	error_t getDuration ( AETIMESPAN * timeSpan )
	{
		if ( _sourceStream == nullptr ) return AEERROR_INVALID_CALL;
		int64_t temp = _sourceStream->length ( _sourceStream->object );
		*timeSpan = AETIMESPAN_initializeWithByteCount ( temp, _wf.bytesPerSec );
		return AEERROR_NOERROR;
	}
	error_t getPosition ( AETIMESPAN * timeSpan )
	{
		if ( _sourceStream == nullptr ) return AEERROR_INVALID_CALL;
		if ( _state == AEPS_STOPPED )
		{
			timeSpan->ticks = 0;
			return AEERROR_NOERROR;
		}
		else if ( _state == AEPS_PAUSED )
		{
			int64_t temp = _sourceStream->tell ( _sourceStream->object );
			*timeSpan = AETIMESPAN_initializeWithByteCount ( temp, _wf.bytesPerSec );
			return AEERROR_NOERROR;
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter ( &currentTime );

		timeSpan->ticks = _sampleTime.ticks + ( ( currentTime.QuadPart - _readedTime ) * 10000000 / _performanceFrequency );

		return AEERROR_NOERROR;
	}
	error_t setPosition ( AETIMESPAN timeSpan )
	{
		if ( _sourceStream == nullptr ) return AEERROR_INVALID_CALL;
		_sourceStream->seek ( _sourceStream->object, ( int64_t ) ( AETIMESPAN_totalSeconds ( timeSpan ) * _wf.bytesPerSec ), AESO_BEGIN );
		return AEERROR_NOERROR;
	}

public:
	error_t getVolume ( float * vol )
	{
		_masteringVoice->GetVolume ( vol );
		return AEERROR_NOERROR;
	}
	error_t setVolume ( float vol )
	{
		if ( FAILED ( _masteringVoice->SetVolume ( vol ) ) )
			return AEERROR_FAIL;
		return AEERROR_NOERROR;
	}

public:
	error_t setSource ( AEAUDIOSTREAM * audioStream )
	{
		if ( audioStream == nullptr )
			return AEERROR_ARGUMENT_IS_NULL;

		stop ();

		AE_releaseInterface ( ( void ** ) &_sourceStream );
		
		if ( _sourceVoice )
		{
			_sourceVoice->Stop ();
			_sourceVoice->DestroyVoice ();
			_sourceVoice = nullptr;
		}

		if ( ISERROR ( audioStream->getWaveFormat ( audioStream->object, &_wf ) ) )
			return AEERROR_FAIL;

		WAVEFORMATEX * pwfx = AEWAVEFORMAT_waveFormatExFromWaveFormat ( &_wf );
		if ( FAILED ( _xaudio2->CreateSourceVoice ( &_sourceVoice, pwfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, this ) ) )
		{
			CoTaskMemFree ( pwfx );
			return AEERROR_FAIL;
		}

		CoTaskMemFree ( pwfx );

		_sourceStream = audioStream;
		AE_retainInterface ( _sourceStream );

		return AEERROR_NOERROR;
	}

public:
	STDMETHOD_ ( void, OnVoiceProcessingPassStart ) ( THIS_ UINT32 BytesRequired ) { }
	STDMETHOD_ ( void, OnVoiceProcessingPassEnd ) ( THIS ) { }
	STDMETHOD_ ( void, OnStreamEnd ) ( THIS ) { }
	STDMETHOD_ ( void, OnBufferStart ) ( THIS_ void* pBufferContext ) { }
	STDMETHOD_ ( void, OnBufferEnd ) ( THIS_ void* pBufferContext )
	{
		if ( _state != AEPS_PLAYING && pBufferContext != ( void * ) 0x12345678 )
			return;

		HRESULT hr;

		BYTE readBuffer [ 524288 ];
		int64_t readed = _sourceStream->read ( _sourceStream->object, &readBuffer [ 0 ], _wf.bytesPerSec / 100 );
		if ( readed == 0 )
		{
			//if ( hr == E_EOF )
			{
				XAUDIO2_BUFFER xBuffer = {};
				xBuffer.Flags = XAUDIO2_END_OF_STREAM;
				_sourceVoice->SubmitSourceBuffer ( &xBuffer );

				_state = AEPS_STOPPED;
				_readedTime = 0;
				_sampleTime.ticks = 0;

				return;
			}
		}
		else if ( readed == -1 )
			return;

		int64_t pos = _sourceStream->tell ( _sourceStream->object );
		_sampleTime = AETIMESPAN_initializeWithSeconds ( pos / ( double ) _wf.bytesPerSec );

		XAUDIO2_BUFFER xBuffer = {};
		xBuffer.pAudioData = readBuffer;
		xBuffer.AudioBytes = ( UINT32 ) readed;
		if ( FAILED ( hr = _sourceVoice->SubmitSourceBuffer ( &xBuffer ) ) )
		{
			return;
		}

		_readedTime = __HRT_GetCounter ();

	}
	STDMETHOD_ ( void, OnLoopEnd ) ( THIS_ void* pBufferContext ) { }
	STDMETHOD_ ( void, OnVoiceError ) ( THIS_ void* pBufferContext, HRESULT Error ) { }

private:
	CComPtr<IXAudio2> _xaudio2;
	IXAudio2MasteringVoice * _masteringVoice;
	IXAudio2SourceVoice * _sourceVoice;

	AEAutoInterface<AEAUDIOSTREAM> _sourceStream;
	AEWAVEFORMAT _wf;

	AEPLAYERSTATE _state;
	int64_t _performanceFrequency;
	int64_t _readedTime;
	AETIMESPAN _sampleTime;
};

error_t AE_createXAudio2AudioPlayer ( int32_t processor, AEAUDIOPLAYER ** ret )
{
	AEAUDIOPLAYER * player = AE_allocInterfaceType ( AEAUDIOPLAYER );
	player->object = new __XAudio2AudioPlayer ( ( XAUDIO2_PROCESSOR ) processor );
	player->free = [] ( void * obj ) { delete reinterpret_cast< __XAudio2AudioPlayer* >( obj ); };
	player->tag = "AudEar XAudio 2 Audio Player";
	player->play = [] ( void * obj ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->play (); };
	player->pause = [] ( void * obj ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->pause (); };
	player->stop = [] ( void * obj ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->stop (); };
	player->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->getDuration ( timeSpan ); };
	player->getPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->getPosition ( timeSpan ); };
	player->setPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->setPosition ( timeSpan ); };
	player->state = [] ( void * obj, AEPLAYERSTATE * ps ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->state ( ps ); };
	player->setSource = [] ( void * obj, AEAUDIOSTREAM * stream ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->setSource ( stream ); };
	player->getVolume = [] ( void * obj, float * vol ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->getVolume ( vol ); };
	player->setVolume = [] ( void * obj, float vol ) { return reinterpret_cast< __XAudio2AudioPlayer* >( obj )->setVolume ( vol ); };

	*ret = player;

	return AEERROR_NOERROR;
}

error_t AE_createXAudio2AudioPlayerWithoutParameters ( AEAUDIOPLAYER ** ret )
{
	return AE_createXAudio2AudioPlayer ( XAUDIO2_DEFAULT_PROCESSOR, ret );
}

#endif