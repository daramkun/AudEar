#include "audear.h"

AEUnknown::AEUnknown ()
	: _refCount ( 1 )
{ }

AEUnknown::~AEUnknown () { }

ULONG AEUnknown::AddRef ()
{
	return InterlockedIncrement ( &_refCount );
}

ULONG AEUnknown::Release ()
{
	ULONG ret = InterlockedDecrement ( &_refCount );
	if ( ret == 0 )
		delete this;
	return ret;
}

#if AEWINDOWS
HRESULT AEUnknown::QueryInterface ( const IID & riid, void ** ppvObject )
{
	if ( riid == __uuidof ( IUnknown ) || riid == __uuidof ( AEUnknown ) )
	{
		*ppvObject = this;
		AddRef ();
		return S_OK;
	}
	return E_FAIL;
}

HRESULT AEDecodedSample::QueryInterface ( const IID & riid, void ** ppvObject )
{
	if ( riid == __uuidof ( AEDecodedSample ) )
	{
		*ppvObject = this;
		AddRef ();
		return S_OK;
	}
	return AEUnknown::QueryInterface ( riid, ppvObject );
}

HRESULT AEAudioDecoder::QueryInterface ( const IID & riid, void ** ppvObject )
{
	if ( riid == __uuidof ( AEAudioDecoder ) )
	{
		*ppvObject = this;
		AddRef ();
		return S_OK;
	}
	return AEUnknown::QueryInterface ( riid, ppvObject );
}

HRESULT AEAudioPlayer::QueryInterface ( const IID & riid, void ** ppvObject )
{
	if ( riid == __uuidof ( AEAudioPlayer ) )
	{
		*ppvObject = this;
		AddRef ();
		return S_OK;
	}
	return AEUnknown::QueryInterface ( riid, ppvObject );
}
#endif
