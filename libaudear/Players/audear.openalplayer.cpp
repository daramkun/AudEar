#include "../audear.h"
#include "../InnerUtilities/Thread.hpp"
#include "../InnerUtilities/HighResolutionTimer.hpp"

#if ( defined ( USE_OPENAL ) && USE_OPENAL )
#	include <AL/al.h>
#	include <AL/alc.h>
#	if AE_PLATFORM_WINDOWS
#		pragma comment ( lib, "OpenAL32.lib" )
#	endif

#define AL_EXT_MCFORMATS									1
#define AL_FORMAT_QUAD8										0x1204
#define AL_FORMAT_QUAD16									0x1205
#define AL_FORMAT_QUAD32									0x1206
#define AL_FORMAT_REAR8										0x1207
#define AL_FORMAT_REAR16									0x1208
#define AL_FORMAT_REAR32									0x1209
#define AL_FORMAT_51CHN8									0x120A
#define AL_FORMAT_51CHN16									0x120B
#define AL_FORMAT_51CHN32									0x120C
#define AL_FORMAT_61CHN8									0x120D
#define AL_FORMAT_61CHN16									0x120E
#define AL_FORMAT_61CHN32									0x120F
#define AL_FORMAT_71CHN8									0x1210
#define AL_FORMAT_71CHN16									0x1211
#define AL_FORMAT_71CHN32									0x1212

#define BUFFER_COUNT										4

#include <vector>
#include <queue>
#include <memory>

struct __ALQUEUEITEM { ALuint bufferId; AETIMESPAN sampleTime; int64_t readedTime; };

class __OpenALAudioPlayer : public __Runnable
{
public:
	__OpenALAudioPlayer ( ALCdevice * device )
		: _alcDevice ( device ), _bufferThread ( this )
		, _state ( AEPS_STOPPED ), _sourceStream ( nullptr )
	{
		_performanceFrequency = __HRT_GetFrequency ();

		ALCint attrList [] = { 0 };
		_alcContext = alcCreateContext ( device, attrList );
		alcMakeContextCurrent ( _alcContext );

		alGenSources ( 1, &_sourceId );
	}
	~__OpenALAudioPlayer ()
	{
		stop ();
		AE_releaseInterface ( ( void ** ) &_sourceStream );

		while ( _buffers.size () > 0 )
		{
			ALuint buffer = _buffers.front ().bufferId;
			alSourceUnqueueBuffers ( _sourceId, 1, &buffer );
			alDeleteBuffers ( 1, &buffer );
			_buffers.pop ();
		}

		alDeleteSources ( 1, &_sourceId );

		alcDestroyContext ( _alcContext );
		alcCloseDevice ( _alcDevice );
	}

public:
	error_t play ()
	{
		if ( _state == AEPS_PLAYING ) return AEERROR_NOERROR;
		if ( _sourceId == 0 ) return AEERROR_INVALID_CALL;

		for ( int i = 0; i < BUFFER_COUNT; ++i )
		{
			ALuint bufferId;
			alGenBuffers ( 1, &bufferId );
			if ( !buffering ( bufferId, _wf.bytesPerSec / 100 ) )
			{
				alDeleteBuffers ( 1, &bufferId );
				break;
			}
		}

		alSourcePlay ( _sourceId );

		_bufferThread.Run ( nullptr );

		_state = AEPS_PLAYING;

		return AEERROR_NOERROR;
	}
	error_t pause ()
	{
		if ( _state == AEPS_PAUSED ) return AEERROR_NOERROR;
		if ( _sourceId == 0 ) return AEERROR_INVALID_CALL;

		_bufferThread.Terminate ( true );
		_bufferThread.Join ();

		alSourceStop ( _sourceId );

		_state = AEPS_PAUSED;

		return AEERROR_NOERROR;
	}
	error_t stop ()
	{
		if ( _state == AEPS_STOPPED ) return AEERROR_NOERROR;
		if ( _sourceId == 0 ) return AEERROR_INVALID_CALL;

		alSourceStop ( _sourceId );

		_bufferThread.Terminate ( true );
		_bufferThread.Join ();

		while ( _buffers.size () > 0 )
		{
			ALuint buffer = _buffers.front ().bufferId;
			alSourceUnqueueBuffers ( _sourceId, 1, &buffer );
			alDeleteBuffers ( 1, &buffer );
			_buffers.pop ();
		}

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

FALLBACK:
		_spinLock.lock ();
		{
			while ( _buffers.size () < 1 )
			{
				_spinLock.unlock ();
				goto FALLBACK;
			}

			__ALQUEUEITEM & buffer = _buffers.front ();

			LARGE_INTEGER currentTime;
			QueryPerformanceCounter ( &currentTime );

			timeSpan->ticks = buffer.sampleTime.ticks + ( ( currentTime.QuadPart - buffer.readedTime ) * 10000000 / _performanceFrequency );
		}
		_spinLock.unlock ();

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
		alGetSourcef ( _sourceId, AL_GAIN, vol );
		return AEERROR_NOERROR;
	}
	error_t setVolume ( float vol )
	{
		alSourcef ( _sourceId, AL_GAIN, vol );
		return AEERROR_NOERROR;
	}

public:
	error_t setSource ( AEAUDIOSTREAM * audioStream )
	{
		if ( audioStream == nullptr )
			return AEERROR_ARGUMENT_IS_NULL;

		stop ();

		AE_releaseInterface ( ( void ** ) &_sourceStream );

		if ( ISERROR ( audioStream->getWaveFormat ( audioStream->object, &_wf ) ) )
			return AEERROR_FAIL;

		switch ( _wf.channels )
		{
			case 2:
				switch ( _wf.bitsPerSample )
				{
					case 16: _bufferFormat = AL_FORMAT_STEREO16; break;
					case 8: _bufferFormat = AL_FORMAT_STEREO8; break;
					default: return AEERROR_NOT_SUPPORTED_FORMAT;
				}
				break;
			case 1:
				switch ( _wf.bitsPerSample )
				{
					case 16: _bufferFormat = AL_FORMAT_MONO16; break;
					case 8: _bufferFormat = AL_FORMAT_MONO8; break;
					default: return AEERROR_NOT_SUPPORTED_FORMAT;
				}
				break;
			case 6:
				switch ( _wf.bitsPerSample )
				{
					case 16: _bufferFormat = AL_FORMAT_51CHN16; break;
					case 32: _bufferFormat = AL_FORMAT_51CHN32; break;
					case 8: _bufferFormat = AL_FORMAT_51CHN8; break;
					default: return AEERROR_NOT_SUPPORTED_FORMAT;
				}
				break;
			case 8:
				switch ( _wf.bitsPerSample )
				{
					case 16: _bufferFormat = AL_FORMAT_71CHN16; break;
					case 32: _bufferFormat = AL_FORMAT_71CHN32; break;
					case 8: _bufferFormat = AL_FORMAT_71CHN8; break;
					default: return AEERROR_NOT_SUPPORTED_FORMAT;
				}
				break;

			default: return AEERROR_NOT_SUPPORTED_FORMAT;
		}

		_sourceStream = audioStream;
		//AE_retainInterface ( _sourceStream );

		return AEERROR_NOERROR;
	}

public:
	virtual void Run ( void * obj, const bool & terminate )
	{
		std::queue<ALuint> proceedBuffers;
		bool innerTerminate = false;
		while ( !terminate && !innerTerminate )
		{
			ALint playerState;
			alGetSourcei ( _sourceId, AL_SOURCE_STATE, &playerState );

			_spinLock.lock ();
			int proceedBufferCount;
			alGetSourcei ( _sourceId, AL_BUFFERS_PROCESSED, &proceedBufferCount );
			for ( int i = 0; i < proceedBufferCount; ++i )
			{
				if ( _buffers.size () < 1 )
					break;

				ALuint bufferId = _buffers.front ().bufferId;
				_buffers.pop ();
				proceedBuffers.push ( bufferId );
			}
			_spinLock.unlock ();

			int actualSize = _wf.bytesPerSec / 100;
			while ( proceedBuffers.size () > 0 )
			{
				if ( !buffering ( proceedBuffers.front (), actualSize ) )
				{
					innerTerminate = true;
					break;
				}
				proceedBuffers.pop ();
			}

			if ( innerTerminate )
				break;

			if ( _state == AEPS_PLAYING && playerState != AL_PLAYING )
				alSourcePlay ( _sourceId );
		}

		do
		{
			ALint state;
			alGetSourcei ( _sourceId, AL_SOURCE_STATE, &state );
			if ( state == AL_STOPPED )
			{
				_state = AEPS_STOPPED;
				break;
			}
		} while ( true );

		while ( proceedBuffers.size () > 0 )
		{
			alDeleteBuffers ( 1, &proceedBuffers.front () );
			proceedBuffers.pop ();
		}
	}

private:
	bool buffering ( ALuint bufferId, int actualSize )
	{
		AEAUDIOBUFFER<uint8_t> readBuffer ( actualSize );
		int64_t readed = _sourceStream->read ( _sourceStream->object, readBuffer, actualSize );
		if ( readed == 0 )
		{
			return false;
		}
		if ( readed == -1 )
		{
			//Sleep ( 1 );
			return true;
		}

		int64_t pos = _sourceStream->tell ( _sourceStream->object );
		AETIMESPAN sampleTime = AETIMESPAN_initializeWithSeconds ( pos / ( double ) _wf.bytesPerSec );

		alSourceUnqueueBuffers ( _sourceId, 1, &bufferId );
		alBufferData ( bufferId, _bufferFormat, &readBuffer [ 0 ], ( ALsizei ) readed, _wf.samplesPerSec );
		alSourceQueueBuffers ( _sourceId, 1, &bufferId );

		_spinLock.lock ();
		_buffers.push ( { bufferId, sampleTime, __HRT_GetCounter () } );
		_spinLock.unlock ();

		return true;
	}

private:
	ALCdevice * _alcDevice;
	ALCcontext * _alcContext;

	AEWAVEFORMAT _wf;
	ALenum _bufferFormat;

	ALuint _sourceId;
	std::queue<__ALQUEUEITEM> _buffers;

	AEAutoInterface<AEAUDIOSTREAM> _sourceStream;
	__Thread _bufferThread;
	__SpinLock _spinLock;

	AEPLAYERSTATE _state;
	int64_t _performanceFrequency;
};

EXTC AEEXP error_t AE_createOpenALAudioPlayer ( const char * deviceName, AEAUDIOPLAYER ** ret )
{
	ALCdevice * device = alcOpenDevice ( deviceName );

	AEAUDIOPLAYER * player = AE_allocInterfaceType ( AEAUDIOPLAYER );
	player->object = new __OpenALAudioPlayer ( device );
	player->free = [] ( void * obj ) { delete reinterpret_cast< __OpenALAudioPlayer* >( obj ); };
	player->tag = "AudEar OpenAL Audio Player";
	player->play = [] ( void * obj ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->play (); };
	player->pause = [] ( void * obj ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->pause (); };
	player->stop = [] ( void * obj ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->stop (); };
	player->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->getDuration ( timeSpan ); };
	player->getPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->getPosition ( timeSpan ); };
	player->setPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->setPosition ( timeSpan ); };
	player->state = [] ( void * obj, AEPLAYERSTATE * ps ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->state ( ps ); };
	player->setSource = [] ( void * obj, AEAUDIOSTREAM * stream ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->setSource ( stream ); };
	player->getVolume = [] ( void * obj, float * vol ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->getVolume ( vol ); };
	player->setVolume = [] ( void * obj, float vol ) { return reinterpret_cast< __OpenALAudioPlayer* >( obj )->setVolume ( vol ); };

	*ret = player;

	return AEERROR_NOERROR;
}

#endif