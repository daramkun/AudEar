#define _CRT_SECURE_NO_WARNINGS
#include "../audear.h"
#include <Windows.h>
#include <atlbase.h>
#include <vector>

class AESpinLock
{
public:
#if AEWINDOWS
	AESpinLock () noexcept
	{
		InitializeSRWLock ( &spinLock );
	}

	~AESpinLock () noexcept
	{

	}
#else
	AESpinLock () noexcept
	{

	}

	~AESpinLock () noexcept
	{

	}
#endif

#if AEWINDOWS
public:
	virtual bool lockRead () noexcept
	{
		AcquireSRWLockShared ( &spinLock );
		return true;
	}
	virtual bool lockWrite () noexcept
	{
		AcquireSRWLockExclusive ( &spinLock );
		return true;
	}

public:
	virtual void unlockRead () noexcept
	{
		ReleaseSRWLockShared ( &spinLock );
	}
	virtual void unlockWrite () noexcept
	{
		ReleaseSRWLockExclusive ( &spinLock );
	}
#else
public:
	virtual bool lockRead () noexcept
	{

	}
	virtual bool lockWrite () noexcept
	{

	}

public:
	virtual void unlockRead () noexcept
	{

	}
	virtual void unlockWrite () noexcept
	{

	}
#endif

private:
#if AEWINDOWS
	SRWLOCK spinLock;
#else

#endif
};

class AEInternalAudioStream : public AEUnknown, public AEAudioStream
{
public:
	AEInternalAudioStream ( AEAudioDecoder * decoder, int maximumBufferSize )
		: _decoder ( decoder ), bufferSize ( maximumBufferSize )
	{
		_decoder->SetReadPosition ( AETimeSpan (), nullptr );
		_decoder->GetWaveFormat ( &pwfx );

		Buffering ();
	}

	~AEInternalAudioStream ()
	{
		free ( pwfx );
	}

public:
#if AEWINDOWS
	HRESULT AEAudioStream::QueryInterface ( const IID & riid, void ** ppvObject )
	{
		if ( riid == __uuidof ( AEAudioStream ) || riid == __uuidof ( IStream ) || riid == __uuidof ( ISequentialStream ) )
		{
			*ppvObject = this;
			AddRef ();
			return S_OK;
		}
		return AEUnknown::QueryInterface ( riid, ppvObject );
	}
#endif
	virtual ULONG __stdcall AddRef () { return AEUnknown::AddRef (); }
	virtual ULONG __stdcall Release () { return AEUnknown::Release (); }

public:
	virtual HRESULT __stdcall GetSourceDecoder ( AEAudioDecoder ** decoder )
	{
		if ( decoder == nullptr )
			return E_FAIL;
		*decoder = _decoder;
		( *decoder )->AddRef ();
		return S_OK;
	}
	virtual HRESULT __stdcall GetCurrentPosition ( AETimeSpan * currentTime )
	{
		*currentTime = this->currentTime;
		return S_OK;
	}

public:
	virtual HRESULT __stdcall Buffering ()
	{
		if ( spinLock.lockWrite () )
		{
			HRESULT hr;
			if ( bufferSize / 10 > _buffer.size () )
			{
				while ( bufferSize > _buffer.size () )
				{
					CComPtr<AEDecodedSample> as;
					AETimeSpan tempReadTime;
					if ( FAILED ( hr = _decoder->GetSample ( &as ) ) )
					{
						if ( hr == E_EOF )
						{
							spinLock.unlockWrite ();
							return E_EOF;
						}
						continue;
					}

					if ( FAILED ( as->GetSampleTime ( &tempReadTime ) ) )
						continue;

					BYTE * readData;
					LONGLONG byteSize;
					if ( FAILED ( as->Lock ( ( void ** ) &readData, &byteSize ) ) )
					{
						spinLock.unlockWrite ();
						return E_FAIL;
					}

					_buffer.insert ( _buffer.end (), readData, readData + byteSize );

					as->Unlock ();
				}
			}
			spinLock.unlockWrite ();
		}

		return S_OK;
	}
	virtual HRESULT __stdcall FlushBuffer ()
	{
		if ( _decoder )
		{
			HRESULT hr;
			if ( FAILED ( hr = _decoder->SetReadPosition ( currentTime, &currentTime ) ) )
				return hr;
		}

		if ( spinLock.lockWrite () )
		{
			_buffer.clear ();
			spinLock.unlockWrite ();
		}
		return S_OK;
	}

public:
	virtual HRESULT __stdcall Read ( void *pv, ULONG cb, ULONG *pcbRead )
	{
		HRESULT hr;

		if ( spinLock.lockWrite () )
		{
			bool eof = false;
			if ( cb > _buffer.size () )
			{
				spinLock.unlockWrite ();
				hr = Buffering ();
				spinLock.lockWrite ();
				if ( hr == E_EOF )
				{
					eof = true;
				}
				else if ( FAILED ( hr ) )
				{
					spinLock.unlockWrite ();
					return hr;
				}
			}

			if ( !eof && _buffer.size () == 0 )
			{
				spinLock.unlockWrite ();
				return E_UNEXPECTED;
			}

			if ( eof && _buffer.size () == 0 )
			{
				spinLock.unlockWrite ();
				return E_EOF;
			}

			size_t sampleSize = min ( cb, _buffer.size () );

			if ( pcbRead != nullptr )
				*pcbRead = ( ULONG ) sampleSize;

			memcpy ( pv, &*_buffer.begin (), sampleSize );

			if ( _buffer.size () == sampleSize )
				_buffer.clear ();
			else
				_buffer.erase ( _buffer.begin (), _buffer.begin () + sampleSize );

			currentTime = currentTime + AETimeSpan::FromSeconds ( sampleSize / ( double ) pwfx->nAvgBytesPerSec );

			spinLock.unlockWrite ();
		}
		return S_OK;
	}
	virtual HRESULT __stdcall Write ( const void *pv, ULONG cb, ULONG *pcbWritten ) { return E_NOTIMPL; }

public:
	virtual HRESULT __stdcall Seek ( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition )
	{
		HRESULT hr;

		AETimeSpan newPos;

		switch ( dwOrigin )
		{
			case STREAM_SEEK_SET:
				if ( FAILED ( hr = _decoder->SetReadPosition ( AETimeSpan::FromByteCount ( dlibMove.QuadPart, pwfx->nAvgBytesPerSec ), &newPos ) ) )
					return hr;
				break;

			case STREAM_SEEK_CUR:
				if ( FAILED ( hr = _decoder->SetReadPosition ( currentTime + AETimeSpan::FromByteCount ( dlibMove.QuadPart, pwfx->nAvgBytesPerSec ), &newPos ) ) )
					return hr;
				break;

			case STREAM_SEEK_END:
				{
					AETimeSpan duration;
					_decoder->GetDuration ( &duration );

					if ( FAILED ( hr = _decoder->SetReadPosition ( duration - AETimeSpan::FromByteCount ( dlibMove.QuadPart, pwfx->nAvgBytesPerSec ), &newPos ) ) )
						return hr;
				}
				break;
		}
		currentTime = newPos;

		if ( plibNewPosition )
			plibNewPosition->QuadPart = newPos.GetBytes ( pwfx->nAvgBytesPerSec );

		_buffer.clear ();

		return S_OK;
	}
	virtual HRESULT __stdcall SetSize ( ULARGE_INTEGER libNewSize ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall CopyTo ( IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall Commit ( DWORD grfCommitFlags ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall Revert ( void ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall LockRegion ( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall UnlockRegion ( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType ) { return E_NOTIMPL; }
	virtual HRESULT __stdcall Stat ( STATSTG *pstatstg, DWORD grfStatFlag )
	{
		HRESULT hr;

		if ( pstatstg == nullptr ) return E_FAIL;

		AETimeSpan duration;
		if ( FAILED ( hr = _decoder->GetDuration ( &duration ) ) )
			return hr;

		pstatstg->cbSize.QuadPart = duration.GetTicks () * pwfx->nAvgBytesPerSec;
		pstatstg->grfMode = STGM_READ;

		return S_OK;
	}
	virtual HRESULT __stdcall Clone ( IStream **ppstm ) { return E_NOTIMPL; }

private:
	CComPtr<AEAudioDecoder> _decoder;
	int bufferSize;
	std::vector<BYTE> _buffer;

	AETimeSpan currentTime;
	WAVEFORMATEX * pwfx;

	AESpinLock spinLock;
};

HRESULT AE_CreateAudioStream ( AEAudioDecoder * decoder, int maximumBufferSize, AEAudioStream ** stream )
{
	*stream = new AEInternalAudioStream ( decoder, maximumBufferSize );
	return S_OK;
}