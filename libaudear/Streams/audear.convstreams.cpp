#include "../audear.h"

#include <memory>
#include <cmath>

static float __conv8To ( uint8_t * buffer, int8_t * increment )
{
	*increment = 1;
	return ( *( ( int8_t* ) buffer ) ) / ( float ) SCHAR_MAX;
}
static float __conv16To ( uint8_t * buffer, int8_t * increment )
{
	*increment = 2;
	return ( *( ( int16_t* ) buffer ) ) / ( float ) SHRT_MAX;
}
static float __conv24To ( uint8_t * buffer, int8_t * increment )
{
	*increment = 3;
	int8_t * arr = ( int8_t * ) buffer;
	int32_t temp = ( ( ( ( int32_t ) arr [ 1 ] ) << 24 ) & 0xff0000 ) + ( ( ( ( int32_t ) arr [ 2 ] ) << 16 ) & 0xff00 ) + arr [ 0 ];
	return temp / 8388607.0f;
}
static float __conv32To ( uint8_t * buffer, int8_t * increment )
{
	*increment = 4;
	return ( *( ( int32_t* ) buffer ) ) / ( float ) INT_MAX;
}

static void __conv8From ( float value, int8_t * to, int8_t * increment )
{
	*increment = 1;
	*to = ( int8_t ) ( value * SCHAR_MAX );
}
static void __conv16From ( float value, int8_t * to, int8_t * increment )
{
	*increment = 2;
	*( ( int16_t* ) to ) = ( int16_t ) ( value * SHRT_MAX );
}
static void __conv24From ( float value, int8_t * to, int8_t * increment )
{
	*increment = 3;
	int temp = ( int ) ( value * 8388607 );
	int8_t f = ( int8_t ) ( ( temp >> 24 ) & 0xff ),
		s = ( int8_t ) ( ( temp >> 16 ) & 0xff ),
		t = ( int8_t ) ( temp & 0xff );
	to [ 1 ] = f;
	to [ 2 ] = s;
	to [ 0 ] = t;
}
static void __conv32From ( float value, int8_t * to, int8_t * increment )
{
	*increment = 4;
	*( ( int32_t* ) to ) = ( int32_t ) ( value * INT_MAX );
}

class __ToIEEEFloatAudioStream
{
public:
	__ToIEEEFloatAudioStream ( AEAUDIOSTREAM * stream, AEWAVEFORMAT wf )
		: _stream ( stream ), _wf ( wf )
	{
		AE_retainInterface ( stream );

		_convwf.channels = wf.channels;
		_convwf.bitsPerSample = 32;
		_convwf.blockAlign = _convwf.channels * 4;
		_convwf.samplesPerSec = wf.samplesPerSec;
		_convwf.bytesPerSec = _convwf.blockAlign * _convwf.samplesPerSec;
		_convwf.audioFormat = AEAF_IEEE_FLOAT;

		switch ( wf.bitsPerSample )
		{
			case 8: _conv = __conv8To; break;
			case 16: _conv = __conv16To; break;
			case 24: _conv = __conv24To; break;
			case 32: _conv = __conv32To; break;
		}
	}
	~__ToIEEEFloatAudioStream ()
	{
		AE_releaseInterface ( ( void ** ) &_stream );
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		*format = _convwf;
		return AEERROR_NOERROR;
	}
	error_t buffering () noexcept
	{
		return _stream->buffering ( _stream->object );
	}

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		int64_t readLen = len / 4 * ( _wf.bitsPerSample / 8 );
		std::shared_ptr<uint8_t []> readBuffer ( new uint8_t [ readLen ] );
		int64_t ret = _stream->read ( _stream->object, &readBuffer [ 0 ], readLen );
		if ( ret == 0 ) return 0;

		int64_t readedLength = ret / ( _wf.bitsPerSample / 8 );

		uint8_t * temp1 = &readBuffer [ 0 ];
		float * temp2 = ( float * ) buffer;
		for ( int i = 0; i < readedLength; ++i )
		{
			int8_t readed;
			temp2 [ i ] = _conv ( temp1, &readed );
			temp1 += readed;
		}

		memcpy ( buffer, temp2, readedLength * 4 );

		return readedLength * 4;
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, ( int64_t ) ( offset / ( double ) _convwf.bytesPerSec * _wf.bytesPerSec ), origin );
	}
	int64_t tell () noexcept
	{
		return ( int64_t ) ( _stream->tell ( _stream->object ) / ( double ) _wf.bytesPerSec * _convwf.bytesPerSec );
	}
	int64_t length () noexcept
	{
		return ( int64_t ) ( _stream->length ( _stream->object ) / ( double ) _wf.bytesPerSec * _convwf.bytesPerSec );
	}

private:
	AEAUDIOSTREAM * _stream;
	AEWAVEFORMAT _wf;
	AEWAVEFORMAT _convwf;
	float ( *_conv )( uint8_t*, int8_t * );
};

error_t AE_createPCMToIEEEFloatAudioStream ( AEAUDIOSTREAM * stream, AEAUDIOSTREAM ** ret )
{
	if ( stream == nullptr || ret == nullptr ) return AEERROR_ARGUMENT_IS_NULL;

	AEWAVEFORMAT wf;
	stream->getWaveFormat ( stream->object, &wf );
	if ( wf.audioFormat == AEAF_IEEE_FLOAT )
	{
		AE_retainInterface ( stream );
		*ret = stream;
		return AEERROR_NOERROR;
	}

	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );

	audioStream->object = new __ToIEEEFloatAudioStream ( stream, wf );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __ToIEEEFloatAudioStream* >( obj ); };
	audioStream->tag = "AudEar PCM to IEEE Float Converter Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __ToIEEEFloatAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __ToIEEEFloatAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __ToIEEEFloatAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __ToIEEEFloatAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __ToIEEEFloatAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __ToIEEEFloatAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}

class __IEEEFloatToPCMAudioStream
{
public:
	__IEEEFloatToPCMAudioStream ( AEAUDIOSTREAM * stream, AEWAVEFORMAT wf, int bps )
		: _stream ( stream ), _wf ( wf ), _bps ( bps )
	{
		AE_retainInterface ( stream );

		switch ( _bps )
		{
			case 8: _conv = __conv8From; break;
			case 16: _conv = __conv16From; break;
			case 24: _conv = __conv24From; break;
			case 32: _conv = __conv32From; break;
		}
	}
	~__IEEEFloatToPCMAudioStream ()
	{
		AE_releaseInterface ( ( void ** ) &_stream );
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		*format = _wf;
		format->audioFormat = AEAF_PCM;
		format->bitsPerSample = _bps;
		format->blockAlign = format->channels * ( _bps / 8 );
		format->bytesPerSec = format->blockAlign * format->samplesPerSec;
		return AEERROR_NOERROR;
	}
	error_t buffering () noexcept
	{
		return _stream->buffering ( _stream->object );
	}

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		int64_t readLen = len / ( _bps / 8 ) * 4;
		std::shared_ptr<float []> tempBuffer ( new float [ readLen / 4 ] );
		int64_t ret = _stream->read ( _stream->object, ( uint8_t* ) &tempBuffer [ 0 ], readLen );
		if ( ret == 0 )
			return 0;
		if ( ret <= -1 )
			return ret;

		int8_t * tempBuffer2 = ( int8_t* ) buffer;
		int64_t readedLen = ret / 4;
		for ( int i = 0; i < readedLen; ++i )
		{
			int8_t inc;
			_conv ( min ( 1, max ( -1, tempBuffer [ i ] ) ), tempBuffer2, &inc );
			tempBuffer2 += inc;
		}

		return readedLen * ( _bps / 8 );
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, offset / ( _wf.bitsPerSample / 8 ) * ( _bps / 8 ), origin );
	}
	int64_t tell () noexcept
	{
		return _stream->tell ( _stream->object ) / ( _wf.bitsPerSample / 8 ) * ( _bps / 8 );
	}
	int64_t length () noexcept
	{
		return _stream->length ( _stream->object ) / ( _wf.bitsPerSample / 8 ) * ( _bps / 8 );
	}

private:
	AEAUDIOSTREAM * _stream;
	AEWAVEFORMAT _wf;
	int _bps;
	void ( *_conv ) ( float, int8_t*, int8_t* );
};

error_t AE_createIEEEFloatToPCMAudioStream ( AEAUDIOSTREAM * stream, int bps, AEAUDIOSTREAM ** ret )
{
	if ( stream == nullptr || ret == nullptr ) return AEERROR_ARGUMENT_IS_NULL;
	if ( !( bps == 8 || bps == 16 || bps == 24 || bps == 32 ) )
		return AEERROR_INVALID_ARGUMENT;

	AEWAVEFORMAT wf;
	stream->getWaveFormat ( stream->object, &wf );
	if ( wf.audioFormat == AEAF_PCM && wf.bitsPerSample == bps )
	{
		AE_retainInterface ( stream );
		*ret = stream;
		return AEERROR_NOERROR;
	}

	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );

	audioStream->object = new __IEEEFloatToPCMAudioStream ( stream, wf, bps );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __IEEEFloatToPCMAudioStream* >( obj ); };
	audioStream->tag = "AudEar IEEE Float to PCM Converter Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __IEEEFloatToPCMAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __IEEEFloatToPCMAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __IEEEFloatToPCMAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __IEEEFloatToPCMAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __IEEEFloatToPCMAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __IEEEFloatToPCMAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}

class __PCMToPCMAudioStream
{
public:
	__PCMToPCMAudioStream ( AEAUDIOSTREAM * stream, AEWAVEFORMAT wf, int bps )
		: _stream ( stream ), _wf ( wf ), _bps ( bps )
	{
		AE_retainInterface ( stream );

		switch ( _wf.bitsPerSample )
		{
			case 8: _read = __conv8To; break;
			case 16: _read = __conv16To; break;
			case 24: _read = __conv24To; break;
			case 32: _read = __conv32To; break;
		}

		switch ( _bps )
		{
			case 8: _write = __conv8From; break;
			case 16: _write = __conv16From; break;
			case 24: _write = __conv24From; break;
			case 32: _write = __conv32From; break;
		}
	}
	~__PCMToPCMAudioStream ()
	{
		AE_releaseInterface ( ( void ** ) &_stream );
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		*format = _wf;
		format->audioFormat = AEAF_PCM;
		format->bitsPerSample = _bps;
		format->blockAlign = format->channels * ( _bps / 8 );
		format->bytesPerSec = format->blockAlign * format->samplesPerSec;
		return AEERROR_NOERROR;
	}
	error_t buffering () noexcept
	{
		return _stream->buffering ( _stream->object );
	}

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		int64_t readLen = len / ( _bps / 8 ) * ( _wf.bitsPerSample / 8 );
		std::shared_ptr<uint8_t []> tempBuffer ( new uint8_t [ readLen ] );
		int64_t ret = _stream->read ( _stream->object, &tempBuffer [ 0 ], readLen );
		if ( ret == 0 )
			return 0;
		if ( ret <= -1 )
			return ret;

		uint8_t * tempBuffer2 = &tempBuffer [ 0 ];
		int8_t * tempBuffer3 = ( int8_t* ) buffer;
		int64_t readedLen = ret / ( _wf.bitsPerSample / 8 );
		for ( int i = 0; i < readedLen; ++i )
		{
			int8_t readInc, writeInc;
			float value = _read ( tempBuffer2, &readInc );
			_write ( value, tempBuffer3, &writeInc );
			tempBuffer2 += readInc;
			tempBuffer3 += writeInc;
		}

		return readedLen * ( _bps / 8 );
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, offset / ( _wf.bitsPerSample / 8 ) * ( _bps / 8 ), origin );
	}
	int64_t tell () noexcept
	{
		return _stream->tell ( _stream->object ) / ( _wf.bitsPerSample / 8 ) * ( _bps / 8 );
	}
	int64_t length () noexcept
	{
		return _stream->length ( _stream->object ) / ( _wf.bitsPerSample / 8 ) * ( _bps / 8 );
	}

private:

private:
	AEAUDIOSTREAM * _stream;
	AEWAVEFORMAT _wf;
	int _bps;

	float ( *_read )( uint8_t*, int8_t * );
	void ( *_write ) ( float, int8_t*, int8_t* );
};

error_t AE_createPCMToPCMAudioStream ( AEAUDIOSTREAM * stream, int bps, AEAUDIOSTREAM ** ret )
{
	if ( stream == nullptr || ret == nullptr ) return AEERROR_ARGUMENT_IS_NULL;
	if ( !( bps == 8 || bps == 16 || bps == 24 || bps == 32 ) )
		return AEERROR_INVALID_ARGUMENT;

	AEWAVEFORMAT wf;
	stream->getWaveFormat ( stream->object, &wf );
	if ( wf.audioFormat == AEAF_IEEE_FLOAT )
		return AEERROR_INVALID_ARGUMENT;
	if ( wf.audioFormat == AEAF_PCM && wf.bitsPerSample == bps )
	{
		AE_retainInterface ( stream );
		*ret = stream;
		return AEERROR_NOERROR;
	}

	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );

	audioStream->object = new __PCMToPCMAudioStream ( stream, wf, bps );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __PCMToPCMAudioStream* >( obj ); };
	audioStream->tag = "AudEar PCM to PCM Converter Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __PCMToPCMAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __PCMToPCMAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __PCMToPCMAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __PCMToPCMAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __PCMToPCMAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __PCMToPCMAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}
