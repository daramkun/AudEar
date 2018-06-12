#include "../audear.h"

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP

class AEWindowsNativeStream : public IStream
{
public:
	AEWindowsNativeStream ( AEBaseStream * stream )
		: stream ( stream )
		, _refcount ( 1 )
	{ }
	~AEWindowsNativeStream () { }

public:
	virtual ULONG __stdcall AddRef ( void )
	{
		return ( ULONG ) InterlockedIncrement ( &_refcount );
	}
	virtual ULONG __stdcall Release ( void )
	{
		ULONG res = ( ULONG ) InterlockedDecrement ( &_refcount );
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
		error_t et;

		bool can;
		if ( FAILED ( et = stream->canRead ( &can ) ) || !can )
			return E_FAIL;
		int64_t ret;
		if ( FAILED ( et = stream->read ( pv, cb, &ret ) ) )
			return E_FAIL;
		if ( pcbRead != nullptr ) *pcbRead = ( ULONG ) ret;
		if ( ret != 0 ) return S_OK;
		return E_FAIL;
	}
	virtual HRESULT __stdcall Write ( const void * pv, ULONG cb, ULONG* pcbWritten )
	{
		bool can;
		if ( FAILED ( stream->canWrite ( &can ) ) || !can )
			return E_FAIL;
		int64_t ret;
		if ( FAILED ( stream->write ( pv, cb, &ret ) ) )
			return E_FAIL;
		if ( pcbWritten != nullptr ) *pcbWritten = ( ULONG ) ret;
		if ( ret != 0 ) return S_OK;
		return E_FAIL;
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
		bool can;
		if ( FAILED ( stream->canSeek ( &can ) ) || !can )
			return E_FAIL;

		AESTREAMSEEK offset;
		switch ( dwOrigin )
		{
			case STREAM_SEEK_SET: offset = kAESTREAMSEEK_SET; break;
			case STREAM_SEEK_CUR: offset = kAESTREAMSEEK_CUR; break;
			case STREAM_SEEK_END: offset = kAESTREAMSEEK_END; break;
			default: return STG_E_INVALIDFUNCTION;
		}

		int64_t ret;
		if ( FAILED ( stream->seek ( offset, liDistanceToMove.QuadPart, &ret ) ) )
			return E_FAIL;
		if ( lpNewFilePointer != nullptr )
			lpNewFilePointer->QuadPart = ret;
		return S_OK;
	}

	virtual HRESULT __stdcall Stat ( STATSTG* pStatstg, DWORD grfStatFlag )
	{
		int64_t length;
		bool canRead, canWrite;

		if ( FAILED ( stream->getLength ( &length ) ) ) return E_FAIL;
		if ( FAILED ( stream->canRead ( &canRead ) ) ) return E_FAIL;
		if ( FAILED ( stream->canWrite ( &canWrite ) ) ) return E_FAIL;

		ZeroMemory ( pStatstg, sizeof ( STATSTG ) );
		pStatstg->type = STGTY_STREAM;
		pStatstg->cbSize.QuadPart = length;
		pStatstg->grfMode = canRead && canWrite ? STGM_READWRITE : ( canRead ? STGM_READ : ( canWrite ? STGM_WRITE : 0 ) );
		
		return S_OK;
	}

private:
	AEAutoPtr<AEBaseStream> stream;
	LONG _refcount;
};

HRESULT AE_convertStream ( AEBaseStream * stream, IStream ** istream )
{
	if ( stream == nullptr || istream == nullptr )
		return E_INVALIDARG;
	*istream = new AEWindowsNativeStream ( stream );
	return S_OK;
}

#endif