#include "../audear.h"

class AEFileStream : public AEBaseStream
{
public:
	AEFileStream ( FILE * fp, bool readonly )
		: _fp ( fp ), _isReadonly ( readonly )
	{ }
	~AEFileStream ()
	{
		fclose ( _fp );
		_fp = nullptr;
	}

public:
	virtual AEERROR read ( OUT void * buffer, int64_t length, OUT int64_t * readed )
	{
		size_t read = fread ( buffer, 1, ( size_t ) length, _fp );
		if ( read == 0 )
			return AEERROR_END_OF_FILE;
		if ( readed )
			*readed = read;
		return AEERROR_SUCCESS;
	}
	virtual AEERROR write ( IN const void * data, int64_t length, OUT int64_t * written )
	{
		if ( _isReadonly ) return AEERROR_INVALID_CALL;
		size_t _written = fwrite ( data, 1, ( size_t ) length, _fp );
		if ( _written == 0 )
			return AEERROR_END_OF_FILE;
		if ( written )
			*written = _written;
		return AEERROR_SUCCESS;
	}
	virtual AEERROR seek ( AESTREAMSEEK offset, int64_t count, OUT int64_t * seeked )
	{
		int origin;
		switch ( offset )
		{
			case kAESTREAMSEEK_SET: origin = SEEK_SET; break;
			case kAESTREAMSEEK_CUR: origin = SEEK_CUR; break;
			case kAESTREAMSEEK_END: origin = SEEK_END; break;
			default: return AEERROR_INVALID_ARGUMENT;
		}

		_fseeki64 ( _fp, count, origin );
		if ( seeked )
			*seeked = _ftelli64 ( _fp );

		return AEERROR_SUCCESS;
	}
	virtual AEERROR flush ()
	{
		if ( 0 != fflush ( _fp ) ) return AEERROR_FAIL;
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR getPosition ( OUT int64_t * pos )
	{
		if ( pos == nullptr )
			return AEERROR_INVALID_ARGUMENT;
		*pos = _ftelli64 ( _fp );
		return AEERROR_SUCCESS;
	}
	virtual AEERROR getLength ( OUT int64_t * len )
	{
		if ( len == nullptr )
			return AEERROR_INVALID_ARGUMENT;

		int64_t old = _ftelli64 ( _fp );
		_fseeki64 ( _fp, 0, SEEK_END );
		*len = _ftelli64 ( _fp );
		_fseeki64 ( _fp, old, SEEK_SET );

		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR canSeek ( OUT bool * can ) { *can = true; return AEERROR_SUCCESS; }
	virtual AEERROR canRead ( OUT bool * can ) { *can = true; return AEERROR_SUCCESS; }
	virtual AEERROR canWrite ( OUT bool * can ) { *can = !_isReadonly; return AEERROR_SUCCESS; }

private:
	FILE * _fp;
	bool _isReadonly;
};

AEERROR AE_createFileStream ( const wchar_t * filename, bool readonly, OUT AEBaseStream ** stream )
{
	FILE * fp;
	_wfopen_s ( &fp, filename, readonly ? L"rb" : L"rb+" );
	if ( fp == nullptr )
		return AEERROR_FAIL;

	*stream = new AEFileStream ( fp, readonly );

	return AEERROR_SUCCESS;
}