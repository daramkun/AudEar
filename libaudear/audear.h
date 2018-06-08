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
	virtual uint32_t retain ();
	virtual uint32_t release ();

private:
	uint32_t _refCount;
};

enum AESTREAMSEEK
{
	kAESTREAMSEEK_SET,
	kAESTREAMSEEK_CUR,
	kAESTREAMSEEK_END,
};

class AEEXP AEBaseStream : public AERefObj
{
public:
	virtual error_t read ( OUT void * buffer, int64_t length, OUT int64_t * readed ) PURE;
	virtual error_t write ( IN const void * data, int64_t length, OUT int64_t * written ) PURE;
	virtual error_t seek ( AESTREAMSEEK offset, int64_t count, OUT int64_t * seeked ) PURE;
	virtual error_t flush () PURE;

public:
	virtual error_t getPosition ( OUT int64_t * pos ) PURE;
	virtual error_t getLength ( OUT int64_t * len ) PURE;

public:
	virtual error_t canSeek ( OUT bool * can ) PURE;
	virtual error_t canRead ( OUT bool * can ) PURE;
	virtual error_t canWrite ( OUT bool * can ) PURE;
};

class AEEXP AEBaseAudioSample : public AERefObj
{
public:
	virtual error_t getSampleTime ( OUT AETimeSpan * time ) PURE;
	virtual error_t getSampleDuration ( OUT AETimeSpan * time ) PURE;

public:
	virtual error_t lock ( OUT void ** buffer, OUT int64_t * length ) PURE;
	virtual error_t unlock () PURE;
};

class AEEXP AEBaseAudioDecoder : public AERefObj
{
public:
	virtual error_t initialize ( IN AEBaseStream * stream ) PURE;

public:
	virtual error_t getWaveFormat ( OUT AEWaveFormat * format ) PURE;
	virtual error_t getDuration ( OUT AETimeSpan * duration ) PURE;

public:
	virtual error_t setReadPosition ( AETimeSpan time ) PURE;

public:
	virtual error_t getSample ( OUT AEBaseAudioSample ** sample ) PURE;
};

class AEEXP AEBaseAudioStream : public AEBaseStream
{
public:
	virtual error_t getBaseDecoder ( OUT AEBaseAudioDecoder ** decoder ) PURE;
	virtual error_t getWaveFormat ( OUT AEWaveFormat * format ) PURE;

public:
	virtual error_t setBufferSize ( AETimeSpan length ) PURE;
	virtual error_t getBufferSize ( OUT AETimeSpan * length ) PURE;

public:
	virtual error_t buffering () PURE;
};

enum AEPLAYERSTATE
{
	kAEPLAYERSTATE_STOPPED,
	kAEPLAYERSTATE_PLAYING,
	kAEPLAYERSTATE_PAUSED,
};

class AEEXP AEBaseAudioPlayer : public AERefObj
{
public:
	virtual error_t setSourceStream ( AEBaseAudioStream * stream ) PURE;

public:
	virtual error_t play () PURE;
	virtual error_t pause () PURE;
	virtual error_t stop () PURE;

public:
	virtual error_t getState ( OUT AEPLAYERSTATE * state ) PURE;
	virtual error_t getDuration ( OUT AETimeSpan * duration ) PURE;

public:
	virtual error_t getPosition ( OUT AETimeSpan * time ) PURE;
	virtual error_t setPosition ( AETimeSpan time ) PURE;

public:
	virtual error_t getVolume ( OUT float * volume ) PURE;
	virtual error_t setVolume ( float volume ) PURE;
};

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