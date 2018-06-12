#include "../audear.h"

class AEMemoryStream : public AEBaseStream
{
public:
	AEMemoryStream ( int64_t capacity )
		: capacity ( capacity ), arrPosition ( 0 ), arrLength ( 0 )
	{
		if ( capacity == 0 )
		{
			arrSize = 64;
			arrLength = 0;
			arr = new uint8_t [ ( size_t ) arrSize ];
		}
		else
			arr = new uint8_t [ ( size_t ) ( arrSize = arrLength = capacity ) ];
	}
	~AEMemoryStream ()
	{
		delete [] arr;
	}

public:
	virtual error_t read ( void * buffer, int64_t length, OUT int64_t * readed )
	{
		int64_t nextPos = arrPosition + length;
		if ( arrPosition + length > arrLength )
		{
			length = arrLength - arrPosition;
			nextPos = arrPosition + length;
		}

		if ( length == 0 && readed )
		{
			*readed = 0;
			return AE_ERROR_END_OF_FILE;
		}

		memcpy ( buffer, arr + arrPosition, ( size_t ) length );

		if ( readed )
			*readed = length;

		return AE_ERROR_SUCCESS;
	}
	virtual error_t write ( const void * buffer, int64_t length, OUT int64_t * written )
	{
		int64_t nextPos = arrPosition + length;
		if ( capacity == 0 )
		{
			if ( nextPos > arrLength )
			{
				if ( nextPos <= arrSize )
					arrLength = nextPos;
				else
				{
					while ( nextPos > arrSize )
						arrSize *= 2;
					uint8_t * temp = new uint8_t [ ( size_t ) arrSize ];
					memcpy ( temp, arr, ( size_t ) arrLength );
					delete [] arr;
					arr = temp;
					arrLength = nextPos;
				}
			}
		}
		else
		{
			if ( arrPosition + 1 == arrLength )
			{
				*written = 0;
				return AE_ERROR_END_OF_FILE;
			}

			if ( arrPosition + length > arrLength )
			{
				length = arrLength - arrPosition;
				nextPos = arrPosition + length;
			}
		}

		memcpy ( arr + arrPosition, buffer, ( size_t ) length );
		arrPosition = nextPos;

		if ( written )
			*written = length;

		return AE_ERROR_SUCCESS;
	}
	virtual error_t seek ( AESTREAMSEEK offset, int64_t count, OUT int64_t * seeked )
	{
		switch ( offset )
		{
			case kAESTREAMSEEK_SET:
				arrPosition = count;
				break;

			case kAESTREAMSEEK_CUR:
				arrPosition += count;
				break;

			case kAESTREAMSEEK_END:
				arrPosition = arrLength - count - 1;
				break;

			default: return AE_ERROR_INVALID_ARGUMENT;
		}
		if ( arrPosition >= arrLength )
			arrPosition = arrLength - 1;
		if ( seeked ) *seeked = arrPosition;

		return AE_ERROR_SUCCESS;
	}
	virtual error_t flush () { return AE_ERROR_SUCCESS; }

public:
	virtual error_t getPosition ( OUT int64_t * pos )
	{
		if ( pos == nullptr ) return AE_ERROR_INVALID_ARGUMENT;
		*pos = arrPosition;
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getLength ( OUT int64_t * len )
	{
		if ( len == nullptr ) return AE_ERROR_INVALID_ARGUMENT;
		*len = arrLength;
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t canSeek ( OUT bool * can ) { *can = true; return AE_ERROR_SUCCESS; }
	virtual error_t canRead ( OUT bool * can ) { *can = true; return AE_ERROR_SUCCESS; }
	virtual error_t canWrite ( OUT bool * can ) { *can = true; return AE_ERROR_SUCCESS; }

private:
	uint8_t * arr;
	int64_t arrSize;

	int64_t arrPosition;
	int64_t arrLength;

	int64_t capacity;
};

error_t AE_createMemoryStream ( int64_t capacity, OUT AEBaseStream ** stream )
{
	*stream = new AEMemoryStream ( capacity );
	return AE_ERROR_SUCCESS;
}