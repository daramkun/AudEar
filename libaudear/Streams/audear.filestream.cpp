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
	virtual error_t read ( OUT void * buffer, int64_t length, OUT int64_t * readed )
	{
		size_t read = fread ( buffer, 1, ( size_t ) length, _fp );
		if ( read == 0 )
			return AE_ERROR_END_OF_FILE;
		if ( readed )
			*readed = read;
		return AE_ERROR_SUCCESS;
	}
	virtual error_t write ( IN const void * data, int64_t length, OUT int64_t * written )
	{
		if ( _isReadonly ) return AE_ERROR_INVALID_CALL;
		size_t _written = fwrite ( data, 1, ( size_t ) length, _fp );
		if ( _written == 0 )
			return AE_ERROR_END_OF_FILE;
		if ( written )
			*written = _written;
		return AE_ERROR_SUCCESS;
	}
	virtual error_t seek ( AESTREAMSEEK offset, int64_t count, OUT int64_t * seeked )
	{
		int origin;
		switch ( offset )
		{
			case kAESTREAMSEEK_SET: origin = SEEK_SET; break;
			case kAESTREAMSEEK_CUR: origin = SEEK_CUR; break;
			case kAESTREAMSEEK_END: origin = SEEK_END; break;
			default: return AE_ERROR_INVALID_ARGUMENT;
		}

		_fseeki64 ( _fp, count, origin );
		if ( seeked )
			*seeked = _ftelli64 ( _fp );

		return AE_ERROR_SUCCESS;
	}
	virtual error_t flush ()
	{
		if ( 0 != fflush ( _fp ) ) return AE_ERROR_FAIL;
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t getPosition ( OUT int64_t * pos )
	{
		if ( pos == nullptr )
			return AE_ERROR_INVALID_ARGUMENT;
		*pos = _ftelli64 ( _fp );
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getLength ( OUT int64_t * len )
	{
		if ( len == nullptr )
			return AE_ERROR_INVALID_ARGUMENT;

		int64_t old = _ftelli64 ( _fp );
		_fseeki64 ( _fp, 0, SEEK_END );
		*len = _ftelli64 ( _fp );
		_fseeki64 ( _fp, old, SEEK_SET );

		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t canSeek ( OUT bool * can ) { *can = true; return AE_ERROR_SUCCESS; }
	virtual error_t canRead ( OUT bool * can ) { *can = true; return AE_ERROR_SUCCESS; }
	virtual error_t canWrite ( OUT bool * can ) { *can = !_isReadonly; return AE_ERROR_SUCCESS; }

private:
	FILE * _fp;
	bool _isReadonly;
};

error_t AE_createFileStream ( const wchar_t * filename, bool readonly, OUT AEBaseStream ** stream )
{
	FILE * fp;
	_wfopen_s ( &fp, filename, readonly ? L"rb" : L"rb+" );
	if ( fp == nullptr )
		return AE_ERROR_FAIL;

	*stream = new AEFileStream ( fp, readonly );

	return AE_ERROR_SUCCESS;
}