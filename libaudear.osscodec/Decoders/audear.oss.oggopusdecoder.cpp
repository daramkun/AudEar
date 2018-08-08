#include "../audear.osscodec.h"

#include <memory>

#include <opusfile.h>
#pragma comment ( lib, "libogg_static.lib" )
#pragma comment ( lib, "opus.lib" )
#pragma comment ( lib, "opusfile.lib" )

class __OggOpusAudioSample
{
public:
	__OggOpusAudioSample ( int8_t * bytes, int64_t length, AETIMESPAN time, AETIMESPAN duration )
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

class __OggOpusAudioDecoder
{
public:
	__OggOpusAudioDecoder ()
		: file ( nullptr ), channels ( 0 ), samplerate ( 0 )
	{
		callbacks.read = [] ( void *datasource, unsigned char *ptr, int nbytes ) -> int
		{
			AESTREAM * stream = ( AESTREAM* ) datasource;
			return ( int ) stream->read ( stream->object, ptr, nbytes );
		};
		callbacks.seek = [] ( void *datasource, int64_t offset, int whence ) -> int
		{
			AESTREAM * stream = ( AESTREAM* ) datasource;
			return ( int ) stream->seek ( stream->object, offset, ( AESEEKORIGIN ) whence );
		};
		callbacks.tell = [] ( void *datasource ) -> int64_t
		{
			AESTREAM * stream = ( AESTREAM* ) datasource;
			return stream->tell ( stream->object );
		};
		callbacks.close = [] ( void *datasource ) -> int
		{
			AESTREAM * stream = ( AESTREAM* ) datasource;
			AE_releaseInterface ( ( void ** ) &stream );
			return 0;
		};
	}
	~__OggOpusAudioDecoder ()
	{
		if ( file )
			op_free ( file );
	}

public:
	error_t initialize ( AESTREAM * stream )
	{
		if ( file )
			op_free ( file );

		AE_retainInterface ( stream );

		int error;
		file = op_open_callbacks ( stream, &callbacks, nullptr, 0, &error );

		if ( error != 0 )
		{
			op_free ( file );
			switch ( error )
			{
				case OP_EREAD: return AEERROR_INVALID_CALL;
				case OP_ENOTAUDIO: return AEERROR_INVALID_ARGUMENT;
				case OP_EVERSION: return AEERROR_NOT_SUPPORTED_FORMAT;
				case OP_EBADHEADER: return AEERROR_INVALID_ARGUMENT;
				case OP_EFAULT: return AEERROR_FAIL;
				default: return AEERROR_UNKNOWN;
			}
		}

		const OpusHead * head = op_head ( file, 0 );
		channels = head->channel_count;
		samplerate = head->input_sample_rate;

		byterate = ( channels * 2 * samplerate );

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
		*timeSpan = AETIMESPAN_initializeWithSeconds ( op_pcm_total ( file, -1 ) / ( float ) samplerate );
		return AEERROR_NOERROR;
	}
	error_t getReadPosition ( AETIMESPAN * timeSpan )
	{
		*timeSpan = AETIMESPAN_initializeWithSeconds ( op_pcm_tell ( file ) / ( float ) samplerate );
		return AEERROR_NOERROR;
	}
	error_t setReadPosition ( AETIMESPAN timeSpan )
	{
		op_pcm_seek ( file, ( int64_t ) ( AETIMESPAN_totalSeconds ( timeSpan ) * samplerate ) );
		return AEERROR_NOERROR;
	}

public:
	error_t readSample ( AEAUDIOSAMPLE ** sample )
	{
		int length = byterate / 100;
		AEAUDIOBUFFER<int8_t> buffer ( byterate );
		int bitstream;
		AETIMESPAN current = AETIMESPAN_initializeWithSeconds ( op_pcm_tell ( file ) / ( float ) samplerate );
		long result = op_read ( file, ( int16_t * ) &buffer [ 0 ], length / 2, &bitstream );

		if ( result == 0 )
			return AEERROR_END_OF_FILE;
		else if ( result == OP_HOLE || result == OP_EBADLINK || result == OP_EINVAL )
			return AEERROR_FAIL;

		result *= ( channels * 2 );

		AEAUDIOSAMPLE * ret = AE_allocInterfaceType ( AEAUDIOSAMPLE );
		ret->object = new __OggOpusAudioSample ( buffer, result, current, AETIMESPAN_initializeWithSeconds ( result / ( float ) byterate ) );
		ret->free = [] ( void * obj ) { delete reinterpret_cast< __OggOpusAudioSample* >( obj ); };
		ret->tag = "AudEar Open Source Software CODEC OggOpus Audio Decoder's Sample";
		ret->getSampleTime = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OggOpusAudioSample* >( obj )->getSampleTime ( timeSpan ); };
		ret->getSampleDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OggOpusAudioSample* >( obj )->getSampleDuration ( timeSpan ); };
		ret->lock = [] ( void * obj, uint8_t ** buffer, int64_t * len ) { return reinterpret_cast< __OggOpusAudioSample* >( obj )->lock ( buffer, len ); };
		ret->unlock = [] ( void * obj ) { return reinterpret_cast< __OggOpusAudioSample* >( obj )->unlock (); };

		*sample = ret;

		return AEERROR_NOERROR;
	}

private:
	OggOpusFile * file;

	int channels, samplerate;
	int byterate;

	OpusFileCallbacks callbacks;
};

error_t AE_createOggOpusAudioDecoder ( AEAUDIODECODER ** ret )
{
	AEAUDIODECODER * decoder = AE_allocInterfaceType ( AEAUDIODECODER );

	decoder->object = new __OggOpusAudioDecoder ();
	decoder->free = [] ( void * obj ) { delete reinterpret_cast< __OggOpusAudioDecoder* >( obj ); };
	decoder->tag = "AudEar Open Source Software CODEC OggOpus Audio Decoder";
	decoder->initialize = [] ( void * obj, AESTREAM * stream ) { return reinterpret_cast< __OggOpusAudioDecoder* >( obj )->initialize ( stream ); };
	decoder->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __OggOpusAudioDecoder* >( obj )->getWaveFormat ( format ); };
	decoder->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OggOpusAudioDecoder* >( obj )->getDuration ( timeSpan ); };
	decoder->getReadPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OggOpusAudioDecoder* >( obj )->getReadPosition ( timeSpan ); };
	decoder->setReadPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __OggOpusAudioDecoder* >( obj )->setReadPosition ( timeSpan ); };
	decoder->readSample = [] ( void * obj, AEAUDIOSAMPLE ** sample ) { return reinterpret_cast< __OggOpusAudioDecoder* >( obj )->readSample ( sample ); };

	*ret = decoder;

	return AEERROR_NOERROR;
}