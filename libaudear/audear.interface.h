#ifndef __AUDEAR_INTERFACE_H__
#define __AUDEAR_INTERFACE_H__

typedef enum AEEXP
{
	AESO_BEGIN,
	AESO_CURRENT,
	AESO_END,
} AESEEKORIGIN;

typedef struct AEEXP
{
	int32_t refCount;
	void * object;
	void ( *free ) ( void * obj );
	const char * tag;

	int64_t ( *read ) ( void * obj, uint8_t * buffer, int64_t len );
	int64_t ( *seek ) ( void * obj, int64_t offset, AESEEKORIGIN origin );
	int64_t ( *tell ) ( void * obj );
	int64_t ( *length ) ( void * obj );
} AESTREAM;

EXTC AEEXP int64_t AESTREAM_read ( AESTREAM * stream, uint8_t * buffer, int64_t len );
EXTC AEEXP int64_t AESTREAM_seek ( AESTREAM * stream, int64_t offset, AESEEKORIGIN origin );
EXTC AEEXP int64_t AESTREAM_tell ( AESTREAM * stream );
EXTC AEEXP int64_t AESTREAM_length ( AESTREAM * stream );

typedef struct AEEXP
{
	int32_t refCount;
	void * object;
	void ( *free ) ( void * obj );
	const char * tag;

	error_t ( *getSampleTime ) ( void * obj, AETIMESPAN * timeSpan );
	error_t ( *getSampleDuration ) ( void * obj, AETIMESPAN * timeSpan );
	error_t ( *lock ) ( void * obj, uint8_t ** buffer, int64_t * length );
	error_t ( *unlock ) ( void * obj );
} AEAUDIOSAMPLE;

EXTC AEEXP error_t AEAUDIOSAMPLE_getSampleTime ( AEAUDIOSAMPLE * sample, AETIMESPAN * timeSpan );
EXTC AEEXP error_t AEAUDIOSAMPLE_getSampleDuration ( AEAUDIOSAMPLE * sample, AETIMESPAN * timeSpan );
EXTC AEEXP error_t AEAUDIOSAMPLE_lock ( AEAUDIOSAMPLE * sample, uint8_t ** buffer, int64_t * length );
EXTC AEEXP error_t AEAUDIOSAMPLE_unlock ( AEAUDIOSAMPLE * sample );

typedef struct AEEXP
{
	int32_t refCount;
	void * object;
	void ( *free ) ( void * obj );
	const char * tag;

	error_t ( *initialize ) ( void * obj, AESTREAM * stream );
	error_t ( *getWaveFormat ) ( void * obj, AEWAVEFORMAT * format );
	error_t ( *getDuration ) ( void * obj, AETIMESPAN * timeSpan );
	error_t ( *getReadPosition ) ( void * obj, AETIMESPAN * timeSpan );
	error_t ( *setReadPosition ) ( void * obj, AETIMESPAN timeSpan );
	error_t ( *readSample ) ( void * obj, AEAUDIOSAMPLE ** sample );
} AEAUDIODECODER;

EXTC AEEXP error_t AEAUDIODECODER_initialize ( AEAUDIODECODER * decoder, AESTREAM * stream );
EXTC AEEXP error_t AEAUDIODECODER_getWaveFormat ( AEAUDIODECODER * decoder, AEWAVEFORMAT * format );
EXTC AEEXP error_t AEAUDIODECODER_getDuration ( AEAUDIODECODER * decoder, AETIMESPAN * timeSpan );
EXTC AEEXP error_t AEAUDIODECODER_getReadPosition ( AEAUDIODECODER * decoder, AETIMESPAN * timeSpan );
EXTC AEEXP error_t AEAUDIODECODER_setReadPosition ( AEAUDIODECODER * decoder, AETIMESPAN timeSpan );
EXTC AEEXP error_t AEAUDIODECODER_readSample ( AEAUDIODECODER * decoder, AEAUDIOSAMPLE ** sample );

typedef struct AEEXP
{
	int32_t refCount;
	void * object;
	void ( *free ) ( void * obj );
	const char * tag;

	error_t ( *getWaveFormat ) ( void * obj, AEWAVEFORMAT * format );
	error_t ( *buffering ) ( void * obj );

	int64_t ( *read ) ( void * obj, uint8_t * buffer, int64_t len );
	int64_t ( *seek ) ( void * obj, int64_t offset, AESEEKORIGIN origin );
	int64_t ( *tell ) ( void * obj );
	int64_t ( *length ) ( void * obj );
} AEAUDIOSTREAM;

EXTC AEEXP error_t AEAUDIOSTREAM_getWaveFormat ( AEAUDIOSTREAM * stream, AEWAVEFORMAT * format );
EXTC AEEXP error_t AEAUDIOSTREAM_buffering ( AEAUDIOSTREAM * stream );
EXTC AEEXP int64_t AEAUDIOSTREAM_read ( AEAUDIOSTREAM * stream, uint8_t * buffer, int64_t len );
EXTC AEEXP int64_t AEAUDIOSTREAM_seek ( AEAUDIOSTREAM * stream, int64_t offset, AESEEKORIGIN origin );
EXTC AEEXP int64_t AEAUDIOSTREAM_tell ( AEAUDIOSTREAM * stream );
EXTC AEEXP int64_t AEAUDIOSTREAM_length ( AEAUDIOSTREAM * stream );

typedef enum AEEXP
{
	AEPS_STOPPED,
	AEPS_PLAYING,
	AEPS_PAUSED,
} AEPLAYERSTATE;

typedef struct AEEXP
{
	int32_t refCount;
	void * object;
	void ( *free ) ( void * obj );
	const char * tag;

	error_t ( *play ) ( void * obj );
	error_t ( *pause ) ( void * obj );
	error_t ( *stop ) ( void * obj );

	error_t ( *state ) ( void * obj, AEPLAYERSTATE * state );
	
	error_t ( *getDuration ) ( void * obj, AETIMESPAN * timeSpan );
	error_t ( *getPosition ) ( void * obj, AETIMESPAN * timeSpan );
	error_t ( *setPosition ) ( void * obj, AETIMESPAN timeSpan );

	error_t ( *getVolume ) ( void * obj, float * vol );
	error_t ( *setVolume ) ( void * obj, float vol );

	error_t ( *setSource ) ( void * obj, AEAUDIOSTREAM * audioStream );
} AEAUDIOPLAYER;

EXTC AEEXP error_t AEAUDIOPLAYER_play ( AEAUDIOPLAYER * player );
EXTC AEEXP error_t AEAUDIOPLAYER_pause ( AEAUDIOPLAYER * player );
EXTC AEEXP error_t AEAUDIOPLAYER_stop ( AEAUDIOPLAYER * player );
EXTC AEEXP error_t AEAUDIOPLAYER_state ( AEAUDIOPLAYER * player, AEPLAYERSTATE * state );
EXTC AEEXP error_t AEAUDIOPLAYER_getDuration ( AEAUDIOPLAYER * player, AETIMESPAN * timeSpan );
EXTC AEEXP error_t AEAUDIOPLAYER_getPosition ( AEAUDIOPLAYER * player, AETIMESPAN * timeSpan );
EXTC AEEXP error_t AEAUDIOPLAYER_setPosition ( AEAUDIOPLAYER * player, AETIMESPAN timeSpan );
EXTC AEEXP error_t AEAUDIOPLAYER_getVolume ( AEAUDIOPLAYER * player, float * vol );
EXTC AEEXP error_t AEAUDIOPLAYER_setVolume ( AEAUDIOPLAYER * player, float vol );
EXTC AEEXP error_t AEAUDIOPLAYER_setSource ( AEAUDIOPLAYER * player, AEAUDIOSTREAM * audioStream );

EXTC AEEXP void* AE_allocInterface ( size_t cb );
#define AE_allocInterfaceType(x)							( x* ) AE_allocInterface ( sizeof ( x ) );
EXTC AEEXP int32_t AE_retainInterface ( void * obj );
EXTC AEEXP int32_t AE_releaseInterface ( void ** obj );

#ifdef __cplusplus
extern "C++"
{
	template<typename T>
	class AEAutoInterface {
	public:
		AEAutoInterface ( T * ptr = nullptr ) noexcept : i ( ptr ) { AE_retainInterface ( ptr ); }
		~AEAutoInterface () noexcept { if ( i ) AE_releaseInterface ( ( void ** ) &i ); }

		void release () noexcept { if ( i ) AE_releaseInterface ( ( void ** ) &i ); }
		void attach ( T * p ) noexcept { if ( i != p ) { release (); i = p; AE_retainInterface ( p ); } }
		T* detach () noexcept { auto temp = i; i = nullptr; return temp; }
		T* get () noexcept { AE_retainInterface ( i ); return i; }

		T * operator= ( T * p ) noexcept { attach ( p ); return ( T* ) i; }
		T * operator= ( const AEAutoInterface<T> & p ) noexcept { attach ( p.i ); return ( T* ) i; }
		operator T * ( ) const noexcept { return reinterpret_cast< T* > ( i ); }
		T ** operator& () noexcept { return ( T** ) ( &i ); }
		T * operator-> () const noexcept { return reinterpret_cast< T* > ( i ); }

	private:
		T * i;
	};
}
#endif

#endif