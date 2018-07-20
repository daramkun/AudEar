#include "../audear.h"
#define USE_SIMD 1
#define USE_PARALLEL 1
#include "../InnerUtilities/TypeConverter.hpp"

#include <memory>
#include <cmath>

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
			case 8: _conv = __TC_int8_to_float; break;
			case 16: _conv = __TC_int16_to_float; break;
			case 24: _conv = __TC_int24_to_float; break;
			case 32: _conv = __TC_int32_to_float; break;
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

		if ( !_conv ( &readBuffer [ 0 ], buffer, ret, len ) )
			return -1;

		return ret / ( _wf.bitsPerSample / 8 ) * 4;
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
	__TypeConverterFunction _conv;
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
			case 8: _conv = __TC_float_to_int8; break;
			case 16: _conv = __TC_float_to_int16; break;
			case 24: _conv = __TC_float_to_int24; break;
			case 32: _conv = __TC_float_to_int32; break;
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

		if ( !_conv ( &tempBuffer [ 0 ], buffer, ret, len ) )
			return -1;

		return ret / 4 * ( _bps / 8 );
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, offset / ( _bps / 8 ) * ( _wf.bitsPerSample / 8 ), origin );
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
	__TypeConverterFunction _conv;
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
			case 8:
				switch ( _bps )
				{
					case 16: _conv = __TC_int8_to_int16; break;
					case 24: _conv = __TC_int8_to_int24; break;
					case 32: _conv = __TC_int8_to_int32; break;
				}
				break;
			case 16:
				switch ( _bps )
				{
					case 8: _conv = __TC_int16_to_int8; break;
					case 24: _conv = __TC_int16_to_int24; break;
					case 32: _conv = __TC_int16_to_int32; break;
				}
				break;
			case 24:
				switch ( _bps )
				{
					case 8: _conv = __TC_int24_to_int8; break;
					case 16: _conv = __TC_int24_to_int16; break;
					case 32: _conv = __TC_int24_to_int32; break;
				}
				break;
			case 32:
				switch ( _bps )
				{
					case 8: _conv = __TC_int32_to_int8; break;
					case 16: _conv = __TC_int32_to_int16; break;
					case 24: _conv = __TC_int32_to_int24; break;
				}
				break;
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

		if ( !_conv ( &tempBuffer [ 0 ], buffer, ret, len ) )
			return -1;

		return ret / ( _wf.bitsPerSample / 8 ) * ( _bps / 8 );
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, offset / ( _bps / 8 ) * ( _wf.bitsPerSample / 8 ), origin );
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

	__TypeConverterFunction _conv;
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

EXTC AEEXP error_t AE_createConverterAudioStream ( AEAUDIOSTREAM * stream, AEAUDIOFORMAT af, int bps, AEAUDIOSTREAM ** ret )
{
	if ( stream == nullptr || ret == nullptr ) return AEERROR_ARGUMENT_IS_NULL;

	if ( !( af == AEAF_PCM && ( bps == 8 || bps == 16 || bps == 24 || bps == 32 ) ) )
		return AEERROR_INVALID_ARGUMENT;
	else if ( !( af == AEAF_IEEE_FLOAT && bps == 32 ) )
		return AEERROR_INVALID_ARGUMENT;

	AEWAVEFORMAT originalFormat;
	if ( ISERROR ( stream->getWaveFormat ( stream->object, &originalFormat ) ) )
		return AEERROR_INVALID_ARGUMENT;

	if ( originalFormat.audioFormat == AEAF_IEEE_FLOAT && af == AEAF_IEEE_FLOAT )
	{
		AE_retainInterface ( stream );
		*ret = stream;
		return AEERROR_NOERROR;
	}
	else if ( originalFormat.audioFormat == AEAF_IEEE_FLOAT && af == AEAF_PCM )
	{
		return AE_createIEEEFloatToPCMAudioStream ( stream, bps, ret );
	}
	else if ( originalFormat.audioFormat == AEAF_PCM && af == AEAF_IEEE_FLOAT )
	{
		return AE_createPCMToIEEEFloatAudioStream ( stream, ret );
	}
	else if ( originalFormat.audioFormat == AEAF_PCM && af == AEAF_PCM )
	{
		if ( originalFormat.bitsPerSample == bps )
		{
			AE_retainInterface ( stream );
			*ret = stream;
			return AEERROR_NOERROR;
		}
		else
			return AE_createPCMToPCMAudioStream ( stream, bps, ret );
	}

	return AEERROR_INVALID_ARGUMENT;
}
