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
		_buffer.resize ( _maxLength );
		_tempBuffer.resize ( _maxLength );
	}

public:
	inline int64_t read ( uint8_t * buffer, int64_t length )
	{
		if ( length > _length )
			length = _length;
		memcpy ( buffer, _buffer, ( size_t ) length );
		memcpy ( _tempBuffer, _buffer + length, ( size_t ) _length - length );

		_buffer.swap ( _tempBuffer );

		_length -= length;
		return length;
	}
	inline void write ( const uint8_t * data, int64_t length )
	{
		int64_t tempLength = _length;
		setLength ( _length + length );
		memcpy ( _buffer + tempLength, data, ( size_t ) length );
	}

public:
	inline int64_t getLength () { return _length; }

	inline void setLength ( int64_t len )
	{
		if ( _maxLength < len )
		{
			int64_t tempLength = _length;
			_maxLength = ( len + 24 ) / 8 * 8;
			_buffer.resizeAndCopy ( _maxLength );
			_tempBuffer.resize ( _maxLength );
		}
		_length = len;
	}

private:
	AEAUDIOBUFFER<uint8_t> _buffer, _tempBuffer;
	int64_t _length, _maxLength;
};

#endif