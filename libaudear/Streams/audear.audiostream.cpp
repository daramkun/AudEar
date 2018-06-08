#include "../audear.h"

#include <vector>

class AEAudioStream : public AEBaseAudioStream
{
public:
	AEAudioStream ( AEBaseAudioDecoder * decoder, AETimeSpan maximumBufferSize )
		: _decoder ( decoder )
	{
		_decoder->setReadPosition ( AETimeSpan () );
		_decoder->getWaveFormat ( &waveFormat );

		bufferSize = maximumBufferSize.getByteCount ( waveFormat );

		buffering ();
	}

	~AEAudioStream () { }

public:
	virtual error_t read ( OUT void * buffer, int64_t length, OUT int64_t * readed )
	{
		error_t et;

		spinLock.lock ();

		bool eof = false;
		if ( length > _buffer.size () )
		{
			spinLock.unlock ();
			et = buffering ();
			spinLock.lock ();
			if ( et == AE_ERROR_END_OF_FILE )
			{
				eof = true;
			}
			else if ( FAILED ( et ) )
			{
				spinLock.unlock ();
				return et;
			}
		}

		if ( !eof && _buffer.size () == 0 )
		{
			spinLock.unlock ();
			return AE_ERROR_FAIL;
		}

		if ( eof && _buffer.size () == 0 )
		{
			spinLock.unlock ();
			return AE_ERROR_END_OF_FILE;
		}

		size_t sampleSize = min ( ( size_t ) length, _buffer.size () );

		if ( readed != nullptr )
			*readed = ( ULONG ) sampleSize;

		memcpy ( buffer, &*_buffer.begin (), sampleSize );

		if ( _buffer.size () == sampleSize )
			_buffer.clear ();
		else
			_buffer.erase ( _buffer.begin (), _buffer.begin () + sampleSize );

		currentTime = currentTime + AETimeSpan::fromByteCount ( sampleSize, waveFormat.getByteRate () );

		spinLock.unlock ();

		return AE_ERROR_SUCCESS;
	}
	virtual error_t write ( IN const void * data, int64_t length, OUT int64_t * written )
	{
		return AE_ERROR_NOT_IMPLEMENTED;
	}
	virtual error_t seek ( AESTREAMSEEK offset, int64_t count, OUT int64_t * seeked )
	{
		error_t et;

		AETimeSpan newPos;
		switch ( offset )
		{
			case kAESTREAMSEEK_SET:
				newPos = AETimeSpan::fromByteCount ( count, waveFormat.getByteRate () );
				break;

			case kAESTREAMSEEK_CUR:
				newPos = currentTime + AETimeSpan::fromByteCount ( count, waveFormat.getByteRate () );
				break;

			case kAESTREAMSEEK_END:
				{
					AETimeSpan duration;
					_decoder->getDuration ( &duration );
					newPos = duration - AETimeSpan::fromByteCount ( count, waveFormat.getByteRate () );
				}
				break;
		}

		if ( FAILED ( et = _decoder->setReadPosition ( newPos ) ) )
			return et;

		currentTime = newPos;

		if ( seeked )
			*seeked = newPos.getByteCount ( waveFormat.getByteRate () );

		_buffer.clear ();

		return AE_ERROR_SUCCESS;
	}
	virtual error_t flush ()
	{
		if ( _decoder )
		{
			error_t et;
			if ( FAILED ( et = _decoder->setReadPosition ( currentTime ) ) )
				return et;
		}

		spinLock.lock ();
		_buffer.clear ();
		spinLock.unlock ();

		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t getPosition ( OUT int64_t * pos )
	{
		if ( pos == nullptr ) return AE_ERROR_INVALID_ARGUMENT;
		*pos = currentTime.getByteCount ( waveFormat );
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getLength ( OUT int64_t * len )
	{
		if ( len == nullptr ) return AE_ERROR_INVALID_ARGUMENT;
		AETimeSpan duration;
		_decoder->getDuration ( &duration );
		*len = duration.getByteCount ( waveFormat );
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t canSeek ( OUT bool * can ) { *can = true; return AE_ERROR_SUCCESS; }
	virtual error_t canRead ( OUT bool * can ) { *can = true; return AE_ERROR_SUCCESS; }
	virtual error_t canWrite ( OUT bool * can ) { *can = false; return AE_ERROR_SUCCESS; }

public:
	virtual error_t getBaseDecoder ( OUT AEBaseAudioDecoder ** decoder )
	{
		if ( decoder == nullptr ) return AE_ERROR_INVALID_ARGUMENT;
		*decoder = _decoder;
		_decoder->retain ();
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getWaveFormat ( OUT AEWaveFormat * format )
	{
		return _decoder->getWaveFormat ( format );
	}

public:
	virtual error_t setBufferSize ( AETimeSpan length )
	{
		bufferSize = length.getByteCount ( waveFormat );
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getBufferSize ( OUT AETimeSpan * length )
	{
		if ( length == nullptr ) return AE_ERROR_INVALID_ARGUMENT;
		*length = bufferSize;
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t buffering ()
	{
		spinLock.lock ();
		error_t et;
		if ( ( size_t ) ( bufferSize / 10 ) > _buffer.size () )
		{
			while ( ( size_t ) bufferSize > _buffer.size () )
			{
				AEAutoPtr<AEBaseAudioSample> as;
				AETimeSpan tempReadTime;
				if ( FAILED ( et = _decoder->getSample ( &as ) ) )
				{
					if ( et == AE_ERROR_END_OF_FILE )
					{
						spinLock.unlock ();
						return AE_ERROR_END_OF_FILE;
					}
					continue;
				}

				if ( FAILED ( as->getSampleTime ( &tempReadTime ) ) )
					continue;

				BYTE * readData;
				LONGLONG byteSize;
				if ( FAILED ( as->lock ( ( void ** ) &readData, &byteSize ) ) )
				{
					spinLock.unlock ();
					return AE_ERROR_FAIL;
				}

				_buffer.insert ( _buffer.end (), readData, readData + byteSize );

				as->unlock ();
			}
		}
		spinLock.unlock ();

		return AE_ERROR_SUCCESS;
	}

private:
	AEAutoPtr<AEBaseAudioDecoder> _decoder;
	int64_t bufferSize;
	std::vector<uint8_t> _buffer;

	AETimeSpan currentTime;
	AEWaveFormat waveFormat;

	AESpinLock spinLock;
};

error_t AE_createAudioStream ( AEBaseAudioDecoder * decoder, AETimeSpan bufferSize, AEBaseAudioStream ** stream )
{
	if ( decoder == nullptr ) return AE_ERROR_INVALID_ARGUMENT;
	*stream = new AEAudioStream ( decoder, bufferSize );
	return AE_ERROR_SUCCESS;
}