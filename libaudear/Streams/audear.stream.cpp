#define _CRT_SECURE_NO_WARNINGS
#include "../audear.h"

#include <cstdio>

error_t AE_createFileStream ( const char * filename, AESTREAM ** ret )
{
	FILE * fp = fopen ( filename, "rb" );
	if ( fp == nullptr )
		return AEERROR_FAIL;

	AESTREAM * stream = AE_allocInterfaceType ( AESTREAM );
	stream->object = fp;
	stream->free = [] ( void * obj )
	{
		fclose ( ( FILE* ) obj );
	};
	stream->tag = "AudEar File Stream";
	stream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) -> int64_t
	{
		return fread ( buffer, 1, ( size_t ) len, ( FILE* ) obj );
	};
	stream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) -> int64_t
	{
		return fseek ( ( FILE* ) obj, ( long ) offset, origin );
	};
	stream->tell = [] ( void * obj ) -> int64_t
	{
		return ftell ( ( FILE* ) obj );
	};
	stream->length = [] ( void * obj ) -> int64_t
	{
		size_t temp = ftell ( ( FILE* ) obj );
		fseek ( ( FILE* ) obj, 0, SEEK_END );
		size_t ret = ftell ( ( FILE* ) obj );
		fseek ( ( FILE* ) obj, ( long ) temp, SEEK_SET );
		return ret;
	};

	*ret = stream;

	return AEERROR_NOERROR;
}

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
class __ComStream : public IStream
{
public:
	__ComStream ( AESTREAM * stream )
		: _stream ( stream ), _refCount ( 1 )
	{
		AE_retainInterface ( _stream );
	}
	~__ComStream ()
	{
		AE_releaseInterface ( ( void ** ) &_stream );
	}

public:
	virtual ULONG __stdcall AddRef ( void )
	{
		return ( ULONG ) InterlockedIncrement ( &_refCount );
	}
	virtual ULONG __stdcall Release ( void )
	{
		ULONG res = ( ULONG ) InterlockedDecrement ( &_refCount );
		if ( res == 0 )
			delete this;
		return res;
	}
	virtual HRESULT __stdcall QueryInterface ( REFIID iid, void ** ppvObject )
	{
		if ( iid == __uuidof( IUnknown )
			|| iid == __uuidof( IStream )
			|| iid == __uuidof( ISequentialStream ) )
		{
			*ppvObject = static_cast<IStream*>( this );
			AddRef ();
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}

public:
	virtual HRESULT __stdcall Read ( void* pv, ULONG cb, ULONG* pcbRead )
	{
		int64_t ret = _stream->read ( _stream->object, ( uint8_t * ) pv, cb );
		if ( pcbRead != nullptr ) *pcbRead = ( ULONG ) ret;
		if ( ret != 0 ) return S_OK;
		return E_FAIL;
	}
	virtual HRESULT __stdcall Write ( const void * pv, ULONG cb, ULONG* pcbWritten )
	{
		return E_NOTIMPL;
	}

public:
	virtual HRESULT __stdcall SetSize ( ULARGE_INTEGER ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall CopyTo ( IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER* ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall Commit ( DWORD ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall Revert ( void ) { return E_NOTIMPL; }

	virtual HRESULT __stdcall LockRegion ( ULARGE_INTEGER, ULARGE_INTEGER, DWORD ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall UnlockRegion ( ULARGE_INTEGER, ULARGE_INTEGER, DWORD ) { return E_NOTIMPL; }

	virtual HRESULT __stdcall Clone ( IStream ** ) { return E_NOTIMPL; }

	virtual HRESULT __stdcall Seek ( LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer )
	{
		int64_t ret = _stream->seek ( _stream->object, liDistanceToMove.QuadPart, ( AESEEKORIGIN ) dwOrigin );
		if ( ret != 0 ) return E_FAIL;
		if ( lpNewFilePointer != nullptr )
			lpNewFilePointer->QuadPart = _stream->tell ( _stream->object );
		return S_OK;
	}

	virtual HRESULT __stdcall Stat ( STATSTG* pStatstg, DWORD grfStatFlag )
	{
		ZeroMemory ( pStatstg, sizeof ( STATSTG ) );
		pStatstg->type = STGTY_STREAM;
		pStatstg->cbSize.QuadPart = _stream->length ( _stream->object );
		pStatstg->grfMode = STGM_READ;
		return S_OK;
	}

private:
	ULONG _refCount;
	AESTREAM * _stream;
	bool _autoRelease;
};

HRESULT AE_convertWindowsComStream ( AESTREAM * stream, IStream ** istream )
{
	if ( stream == nullptr || istream == nullptr )
		return E_FAIL;

	*istream = new __ComStream ( stream );
	
	return S_OK;
}
#endif