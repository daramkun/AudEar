#ifndef __AUDEAR_H__
#define __AUDEAR_H__

#include "audear.preprocessor.h"
#include "audear.struct.h"

#include "audear.platform.h"

class AEEXP AERefObj
{
public:
	AERefObj ();
	virtual ~AERefObj ();
	virtual uint32_t STDCALL retain ();
	virtual uint32_t STDCALL release ();

private:
	uint32_t _refCount;
};

EXTC uint32_t AEEXP STDCALL AERefObj_retain ( AERefObj * obj );
EXTC uint32_t AEEXP STDCALL AERefObj_release ( AERefObj * obj );

enum AESTREAMSEEK : int32_t
{
	kAESTREAMSEEK_SET = 0,
	kAESTREAMSEEK_CUR = 1,
	kAESTREAMSEEK_END = 2,
};

class AEEXP AEBaseStream : public AERefObj
{
public:
	virtual AEERROR STDCALL read ( OUT void * buffer, int64_t length, OUT int64_t * readed ) PURE;
	virtual AEERROR STDCALL write ( IN const void * data, int64_t length, OUT int64_t * written ) PURE;
	virtual AEERROR STDCALL seek ( AESTREAMSEEK offset, int64_t count, OUT int64_t * seeked ) PURE;
	virtual AEERROR STDCALL flush () PURE;

public:
	virtual AEERROR STDCALL getPosition ( OUT int64_t * pos ) PURE;
	virtual AEERROR STDCALL getLength ( OUT int64_t * len ) PURE;

public:
	virtual AEERROR STDCALL canSeek ( OUT bool * can ) PURE;
	virtual AEERROR STDCALL canRead ( OUT bool * can ) PURE;
	virtual AEERROR STDCALL canWrite ( OUT bool * can ) PURE;
};

EXTC AEEXP AEERROR STDCALL AEBaseStream_read ( AEBaseStream * stream, void * buffer, int64_t length, int64_t * readed );
EXTC AEEXP AEERROR STDCALL AEBaseStream_write ( AEBaseStream * stream, const void * data, int64_t length, int64_t * written );
EXTC AEEXP AEERROR STDCALL AEBaseStream_seek ( AEBaseStream * stream, AESTREAMSEEK offset, int64_t count, int64_t * seeked );
EXTC AEEXP AEERROR STDCALL AEBaseStream_flush ( AEBaseStream * stream );
EXTC AEEXP AEERROR STDCALL AEBaseStream_getPosition ( AEBaseStream * stream, int64_t * pos );
EXTC AEEXP AEERROR STDCALL AEBaseStream_getLength ( AEBaseStream * stream, int64_t * len );
EXTC AEEXP AEERROR STDCALL AEBaseStream_canSeek ( AEBaseStream * stream, bool * can );
EXTC AEEXP AEERROR STDCALL AEBaseStream_canRead ( AEBaseStream * stream, bool * can );
EXTC AEEXP AEERROR STDCALL AEBaseStream_canWrite ( AEBaseStream * stream, bool * can );

struct AEEXP AECustomStreamCallback
{
	void * user_data;
	AEERROR ( *dispose ) ( void * user_data );
	AEERROR ( *read ) ( void * user_data, void * buffer, int64_t length, int64_t * readed );
	AEERROR ( *write ) ( void * user_data, const void * data, int64_t length, int64_t * written );
	AEERROR ( *seek ) ( void * user_data, AESTREAMSEEK offset, int64_t count, int64_t * seeked );
	AEERROR ( *flush ) ( void * user_data );
	AEERROR ( *getPosition ) ( void * user_data, int64_t * pos );
	AEERROR ( *getLength ) ( void * user_data, int64_t * len );
	AEERROR ( *canSeek ) ( void * user_data, bool * can );
	AEERROR ( *canRead ) ( void * user_data, bool * can );
	AEERROR ( *canWrite ) ( void * user_data, bool * can );
};
EXTC AEEXP AEERROR STDCALL AE_createCustomCallbackStream ( const AECustomStreamCallback * callback, AEBaseStream ** stream );

class AEEXP AEBaseAudioSample : public AERefObj
{
public:
	virtual AEERROR STDCALL getSampleTime ( OUT AETimeSpan * time ) PURE;
	virtual AEERROR STDCALL getSampleDuration ( OUT AETimeSpan * time ) PURE;

public:
	virtual AEERROR STDCALL lock ( OUT void ** buffer, OUT int64_t * length ) PURE;
	virtual AEERROR STDCALL unlock () PURE;
};

EXTC AEEXP AEERROR STDCALL AEBaseAudioSample_getSampleTime ( AEBaseAudioSample * sample, AETimeSpan * time );
EXTC AEEXP AEERROR STDCALL AEBaseAudioSample_getSampleDuration ( AEBaseAudioSample * sample, AETimeSpan * time );
EXTC AEEXP AEERROR STDCALL AEBaseAudioSample_lock ( AEBaseAudioSample * sample, void ** buffer, int64_t * length );
EXTC AEEXP AEERROR STDCALL AEBaseAudioSample_unlock ( AEBaseAudioSample * sample );

class AEEXP AEBaseAudioDecoder : public AERefObj
{
public:
	virtual AEERROR STDCALL initialize ( IN AEBaseStream * stream ) PURE;

public:
	virtual AEERROR STDCALL getWaveFormat ( OUT AEWaveFormat * format ) PURE;
	virtual AEERROR STDCALL getDuration ( OUT AETimeSpan * duration ) PURE;

public:
	virtual AEERROR STDCALL setReadPosition ( AETimeSpan time ) PURE;

public:
	virtual AEERROR STDCALL getSample ( OUT AEBaseAudioSample ** sample ) PURE;
};

EXTC AEEXP AEERROR STDCALL AEBaseAudioDecoder_initialize ( AEBaseAudioDecoder * decoder, AEBaseStream * stream );
EXTC AEEXP AEERROR STDCALL AEBaseAudioDecoder_getWaveFormat ( AEBaseAudioDecoder * decoder, AEWaveFormat * format );
EXTC AEEXP AEERROR STDCALL AEBaseAudioDecoder_getDuration ( AEBaseAudioDecoder * decoder, AETimeSpan * duration );
EXTC AEEXP AEERROR STDCALL AEBaseAudioDecoder_setReadPosition ( AEBaseAudioDecoder * decoder, AETimeSpan time );
EXTC AEEXP AEERROR STDCALL AEBaseAudioDecoder_getSample ( AEBaseAudioDecoder * decoder, AEBaseAudioSample ** sample );

class AEEXP AEBaseAudioStream : public AEBaseStream
{
public:
	virtual AEERROR STDCALL getBaseDecoder ( OUT AEBaseAudioDecoder ** decoder ) PURE;
	virtual AEERROR STDCALL getWaveFormat ( OUT AEWaveFormat * format ) PURE;

public:
	virtual AEERROR STDCALL setBufferSize ( AETimeSpan length ) PURE;
	virtual AEERROR STDCALL getBufferSize ( OUT AETimeSpan * length ) PURE;

public:
	virtual AEERROR STDCALL buffering () PURE;
};

EXTC AEEXP AEERROR STDCALL AEBaseAudioStream_getBaseDecoder ( AEBaseAudioStream * stream, AEBaseAudioDecoder ** decoder );
EXTC AEEXP AEERROR STDCALL AEBaseAudioStream_getWaveFormat ( AEBaseAudioStream * stream, AEWaveFormat * format );
EXTC AEEXP AEERROR STDCALL AEBaseAudioStream_setBufferSize ( AEBaseAudioStream * stream, AETimeSpan length );
EXTC AEEXP AEERROR STDCALL AEBaseAudioStream_getBufferSize ( AEBaseAudioStream * stream, AETimeSpan * length );
EXTC AEEXP AEERROR STDCALL AEBaseAudioStream_buffering ( AEBaseAudioStream * stream );

enum AEPLAYERSTATE
{
	kAEPLAYERSTATE_STOPPED,
	kAEPLAYERSTATE_PLAYING,
	kAEPLAYERSTATE_PAUSED,
};

class AEEXP AEBaseAudioPlayer : public AERefObj
{
public:
	virtual AEERROR STDCALL setSourceStream ( AEBaseAudioStream * stream ) PURE;

public:
	virtual AEERROR STDCALL play () PURE;
	virtual AEERROR STDCALL pause () PURE;
	virtual AEERROR STDCALL stop () PURE;

public:
	virtual AEERROR STDCALL getState ( OUT AEPLAYERSTATE * state ) PURE;
	virtual AEERROR STDCALL getDuration ( OUT AETimeSpan * duration ) PURE;

public:
	virtual AEERROR STDCALL getPosition ( OUT AETimeSpan * time ) PURE;
	virtual AEERROR STDCALL setPosition ( AETimeSpan time ) PURE;

public:
	virtual AEERROR STDCALL getVolume ( OUT float * volume ) PURE;
	virtual AEERROR STDCALL setVolume ( float volume ) PURE;
};

EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_setSourceStream ( AEBaseAudioPlayer * player, AEBaseAudioStream * stream );
EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_play ( AEBaseAudioPlayer * player );
EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_pause ( AEBaseAudioPlayer * player );
EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_stop ( AEBaseAudioPlayer * player );
EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_getState ( AEBaseAudioPlayer * player, AEPLAYERSTATE * state );
EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_getDuration ( AEBaseAudioPlayer * player, AETimeSpan * duration );
EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_getPosition ( AEBaseAudioPlayer * player, AETimeSpan * time );
EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_setPosition ( AEBaseAudioPlayer * player, AETimeSpan time );
EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_getVolume ( AEBaseAudioPlayer * player, float * volume );
EXTC AEEXP AEERROR STDCALL AEBaseAudioPlayer_setVolume ( AEBaseAudioPlayer * player, float volume );

template<typename T> class AEAutoPtr
{
public:
	AEAutoPtr ( T * p = nullptr ) : ptr ( p )
	{
		static_assert ( std::is_base_of<AERefObj, T>::value, "Template parameter not derived from AERefObj." );
		if ( p )
			ptr->retain ();
	}
	AEAutoPtr ( const AEAutoPtr<T> & ap ) : ptr ( ap.ptr )
	{
		static_assert ( std::is_base_of<AERefObj, T>::value, "Template parameter not derived from AERefObj." );
		ptr->retain ();
	}
	~AEAutoPtr () { if ( ptr ) ptr->release (); }

	void release () noexcept { if ( ptr ) { ptr->release (); ptr = nullptr; } }
	void attach ( T* p ) { if ( ptr != p ) { if ( ptr ) ptr->release (); ptr = p; p->retain (); } }
	T* detach () { auto temp = ptr; ptr = nullptr; return temp; }
	T* get () { ptr->retain (); return ptr; }

	T* operator= ( T* p ) { attach ( p ); return ( T* ) ptr; }
	T* operator= ( const AEAutoPtr<T> & ap ) { set ( ap.ptr ); return ptr; }
	operator T* ( ) const { return static_cast<T*> ( ptr ); }
	T** operator& ()
	{
		return ( T** ) ( &ptr );
	}
	T* operator-> () const { return static_cast<T*> ( ptr ); }

private:
	AERefObj * ptr;
};

#include "audear.streams.h"
#include "audear.decoders.h"
#include "audear.players.h"

#endif