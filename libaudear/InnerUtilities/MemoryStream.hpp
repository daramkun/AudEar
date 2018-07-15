#ifndef __AUDEAR_MEMORYSTREAM_HPP__
#define __AUDEAR_MEMORYSTREAM_HPP__

#include <cstdint>
#include <cstring>

class __MemoryStream
{
public:
	inline __MemoryStream ()
		: _maxLength ( 48000 * 2 * 2 ), _length ( 0 )
	{
		_buffer = new uint8_t [ _maxLength ];
		_tempBuffer = new uint8_t [ _maxLength ];
	}
	inline ~__MemoryStream ()
	{
		delete [] _tempBuffer;
		delete [] _buffer;
		_tempBuffer = nullptr;
		_buffer = nullptr;
	}

public:
	inline int64_t read ( uint8_t * buffer, int64_t length )
	{
		if ( length > _length )
			length = _length;
		memcpy ( buffer, _buffer, length );
		memcpy ( _tempBuffer, _buffer + length, _length - length );

		uint8_t * temp = _buffer;
		_buffer = _tempBuffer;
		_tempBuffer = temp;

		_length -= length;
		return length;
	}
	inline void write ( const uint8_t * data, int64_t length )
	{
		int64_t tempLength = _length;
		setLength ( _length + length );
		memcpy ( _buffer + tempLength, data, length );
	}

public:
	inline int64_t getLength () { return _length; }

	inline void setLength ( int64_t len )
	{
		if ( _maxLength < len )
		{
			uint8_t * temp = _buffer;
			int64_t tempLength = _length;
			_maxLength = ( len + 24 ) / 8 * 8;
			_buffer = new uint8_t [ _maxLength ];
			memcpy ( _buffer, temp, tempLength );
			delete [] temp;

			delete [] _tempBuffer;
			_tempBuffer = new uint8_t [ _maxLength ];
		}
		_length = len;
	}

private:
	uint8_t * _buffer, *_tempBuffer;
	int64_t _length, _maxLength;
};

#endif