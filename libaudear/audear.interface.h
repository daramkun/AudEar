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

EXTC AEEXP void* AE_allocInterface ( size_t cb );
#define AE_allocInterfaceType(x)							( x* ) AE_allocInterface ( sizeof ( x ) );
EXTC AEEXP int32_t AE_retainInterface ( void * obj );
EXTC AEEXP int32_t AE_releaseInterface ( void ** obj );

#ifdef __cplusplus
extern "C++"
{
	template<typename T>
	class AEAutoInterface
	{
	public:
		AEAutoInterface ( T * ptr = nullptr )
		{
			i = ptr;
			AE_retainInterface ( ptr );
		}
		~AEAutoInterface ()
		{
			if ( i )
				AE_releaseInterface ( ( void ** ) &i );
		}

	public:
		void release () noexcept
		{
			if ( i )
				AE_releaseInterface ( ( void ** ) &i );
			i = nullptr;
		}
		void attach ( T * p )
		{
			if ( i != p )
			{
				release ();

				i = p;
				AE_retainInterface ( p );
			}
		}
		T* detach ()
		{
			auto temp = i;
			i = nullptr;
			return temp;
		}

		T* get ()
		{
			AE_retainInterface ( i );
			return i;
		}

	public:
		T * operator= ( T * p ) { attach ( p ); return ( T* ) i; }
		T * operator= ( const AEAutoInterface<T> & p ) { attach ( p.i ); return ( T* ) i; }
		operator T * ( ) const { return reinterpret_cast< T* > ( i ); }
		T ** operator& () { return ( T** ) ( &i ); }
		T * operator-> () const { return reinterpret_cast< T* > ( i ); }

	private:
		T * i;
	};
}
#endif

#endif