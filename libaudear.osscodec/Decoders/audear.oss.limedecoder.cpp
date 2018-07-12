#include "../audear.osscodec.h"
#include "audear.oss.limedecoder.utility.h"

#include <memory>

#include <lame/lame.h>
#pragma comment ( lib, "libmp3lame-static.lib" )
#pragma comment ( lib, "libmpghip-static.lib" )

class __LimeAudioSample
{
public:
	__LimeAudioSample ( int16_t * bytes, int64_t length, AETIMESPAN time, AETIMESPAN duration )
		: bytes ( new uint8_t [ length ] ), length ( length ), time ( time ), duration ( duration )
	{
		memcpy ( &this->bytes [ 0 ], bytes, length );
	}

public:
	error_t getSampleTime ( AETIMESPAN * timeSpan ) { *timeSpan = time; return AEERROR_NOERROR; }
	error_t getSampleDuration ( AETIMESPAN * timeSpan ) { *timeSpan = duration; return AEERROR_NOERROR; }

public:
	error_t lock ( uint8_t ** buffer, int64_t * length )
	{
		*buffer = &bytes [ 0 ];
		*length = this->length;
		return AEERROR_NOERROR;
	}
	error_t unlock ()
	{
		return AEERROR_NOERROR;
	}

private:
	std::shared_ptr<uint8_t []> bytes;
	int64_t length;
	AETIMESPAN time, duration;
};

class __LimeAudioDecoder
{
public:
	__LimeAudioDecoder ()
		: hip ( 0 ), stream ( nullptr )
	{

	}
	~__LimeAudioDecoder ()
	{

	}

public:
	error_t initialize ( AESTREAM * stream )
	{
		cache_chunk_position ( stream, posizeCache, &channels, &samplerate );

		if ( posizeCache.size () == 0 )
			return AEERROR_NOT_SUPPORTED_FORMAT;

		if ( stream->seek ( stream->object, 0, AESO_BEGIN ) == -1 )
			return AEERROR_FAIL;

		this->stream.release ();
		if ( hip != 0 )
			hip_decode_exit ( hip );

		this->stream = stream;

		hip = hip_decode_init ();

		byterate = channels * 2 * samplerate;

		nextIndex = 0;

		return AEERROR_NOERROR;
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format )
	{
		*format = AEWAVEFORMAT_initialize ( channels, 16, samplerate, AEAF_PCM );
		return AEERROR_NOERROR;
	}
	error_t getDuration ( AETIMESPAN * timeSpan )
	{
		*timeSpan = AETIMESPAN_initializeWithSeconds ( posizeCache.size () * 0.026125 );
		return AEERROR_NOERROR;
	}
	error_t getReadPosition ( AETIMESPAN * timeSpan )
	{
		*timeSpan = AETIMESPAN_initializeWithSeconds ( nextIndex * 0.026125f );
		return AEERROR_NOERROR;
	}
	error_t setReadPosition ( AETIMESPAN timeSpan )
	{
		nextIndex = ( int ) round ( AETIMESPAN_totalSeconds ( timeSpan ) / 0.026125 );
		auto posize = posizeCache.at ( nextIndex++ );
		if ( stream->seek ( stream->object, posize.position, AESO_BEGIN ) == -1 )
			return AEERROR_FAIL;
		return AEERROR_NOERROR;
	}

public:
	error_t readSample ( AEAUDIOSAMPLE ** sample )
	{
		uint8_t buffer [ 48000 ];
		int16_t lrPCM [ 2 ] [ 48000 ];
		int16_t mixedPCM [ 48000 * 2 ];
		int64_t readed;

		if ( nextIndex >= posizeCache.size () )
			return AEERROR_END_OF_FILE;

		auto posize = posizeCache.at ( nextIndex++ );

		int64_t pos = stream->tell ( stream->object );

		if ( pos != posize.position )
			if ( stream->seek ( stream->object, posize.position, AESO_BEGIN ) == -1 )
				return AEERROR_FAIL;

		if ( ( readed = stream->read ( stream->object, buffer, posize.size ) ) == 0 )
			return AEERROR_FAIL;

		int result = hip_decode1 ( hip, buffer, 0, lrPCM [ 0 ], lrPCM [ 1 ] );
		if ( result == 0 )
		{
			result = hip_decode1 ( hip, buffer, min ( posize.size, readed ), lrPCM [ 0 ], lrPCM [ 1 ] );
			if ( result == 0 )
				return AEERROR_FAIL;
		}

		for ( int j = 0; j < result; ++j )
		{
			if ( channels == 2 )
			{
				mixedPCM [ j * 2 ] = lrPCM [ 0 ] [ j ];
				mixedPCM [ j * 2 + 1 ] = lrPCM [ 1 ] [ j ];
			}
			else mixedPCM [ j ] = lrPCM [ 0 ] [ j ];
		}

		AEAUDIOSAMPLE * ret = AE_allocInterfaceType ( AEAUDIOSAMPLE );
		ret->object = new __LimeAudioSample ( mixedPCM, result * sizeof ( int16_t ) * channels,
			AETIMESPAN_initializeWithSeconds ( nextIndex - 1 * 0.026125 ), AETIMESPAN_initializeWithSeconds ( result / ( float ) byterate ) );
		ret->free = [] ( void * obj ) { delete reinterpret_cast< __LimeAudioSample* >( obj ); };
		ret->tag = "AudEar Open Source Software CODEC LimeMP3 Audio Decoder's Sample";
		ret->getSampleTime = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __LimeAudioSample* >( obj )->getSampleTime ( timeSpan ); };
		ret->getSampleDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __LimeAudioSample* >( obj )->getSampleDuration ( timeSpan ); };
		ret->lock = [] ( void * obj, uint8_t ** buffer, int64_t * len ) { return reinterpret_cast< __LimeAudioSample* >( obj )->lock ( buffer, len ); };
		ret->unlock = [] ( void * obj ) { return reinterpret_cast< __LimeAudioSample* >( obj )->unlock (); };

		*sample = ret;

		return AEERROR_NOERROR;
	}

private:
	hip_t hip;

	int channels, samplerate, byterate;

	AEAutoInterface<AESTREAM> stream;
	std::vector<POSITIONSIZE> posizeCache;
	int nextIndex;

	int64_t startPoint;
};

error_t AE_createLimeMP3AudioDecoder ( AEAUDIODECODER ** ret )
{
	AEAUDIODECODER * decoder = AE_allocInterfaceType ( AEAUDIODECODER );

	decoder->object = new __LimeAudioDecoder ();
	decoder->free = [] ( void * obj ) { delete reinterpret_cast< __LimeAudioDecoder* >( obj ); };
	decoder->tag = "AudEar Open Source Software CODEC LimeMP3 Audio Decoder";
	decoder->initialize = [] ( void * obj, AESTREAM * stream ) { return reinterpret_cast< __LimeAudioDecoder* >( obj )->initialize ( stream ); };
	decoder->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __LimeAudioDecoder* >( obj )->getWaveFormat ( format ); };
	decoder->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __LimeAudioDecoder* >( obj )->getDuration ( timeSpan ); };
	decoder->getReadPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __LimeAudioDecoder* >( obj )->getReadPosition ( timeSpan ); };
	decoder->setReadPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __LimeAudioDecoder* >( obj )->setReadPosition ( timeSpan ); };
	decoder->readSample = [] ( void * obj, AEAUDIOSAMPLE ** sample ) { return reinterpret_cast< __LimeAudioDecoder* >( obj )->readSample ( sample ); };

	*ret = decoder;

	return AEERROR_NOERROR;
}