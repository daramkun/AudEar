#include "../audear.osscodec.h"

#include <memory>

#define FLAC__NO_DLL
#include <FLAC/all.h>
#pragma comment ( lib, "libogg_static.lib" )
#pragma comment ( lib, "libFLAC_static.lib" )

class __FLACAudioSample
{
public:
	__FLACAudioSample ( int8_t * bytes, int64_t length, AETIMESPAN time, AETIMESPAN duration )
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

inline void WriteToBuffer16 ( int16_t * buffer, FLAC__int16 data )
{
	union {
		int16_t word;
		int8_t arr [ 2 ];
	} temp;
	temp.arr [ 0 ] = ( int8_t ) ( data & 0xff );
	temp.arr [ 1 ] = ( int8_t ) ( ( data >> 8 ) & 0xff );
	*buffer = temp.word;
}

typedef FLAC__StreamDecoderInitStatus ( *FLAC_INIT ) (
	FLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderReadCallback read_callback,
	FLAC__StreamDecoderSeekCallback seek_callback,
	FLAC__StreamDecoderTellCallback tell_callback,
	FLAC__StreamDecoderLengthCallback length_callback,
	FLAC__StreamDecoderEofCallback eof_callback,
	FLAC__StreamDecoderWriteCallback write_callback,
	FLAC__StreamDecoderMetadataCallback metadata_callback,
	FLAC__StreamDecoderErrorCallback error_callback,
	void *client_data
);

class __FLACAudioDecoder
{
public:
	__FLACAudioDecoder ( FLAC_INIT initFunc )
		: file ( nullptr ), initFunc ( initFunc )
	{
		isOggContainer = ( initFunc == FLAC__stream_decoder_init_ogg_stream );
	}
	~__FLACAudioDecoder ()
	{
		if ( file )
		{
			FLAC__stream_decoder_delete ( file );
			file = nullptr;
		}
	}

public:
	error_t initialize ( AESTREAM * stream )
	{
		if ( file )
		{
			FLAC__stream_decoder_delete ( file );
			file = nullptr;
		}

		AE_retainInterface ( stream );
		this->stream = stream;

		channels = bitsPerSample = samplerate = byterate = 0;

		file = FLAC__stream_decoder_new ();
		auto result = initFunc ( file,
			[] ( const FLAC__StreamDecoder *decoder, FLAC__byte buffer [], size_t *bytes, void *client_data )
			-> FLAC__StreamDecoderReadStatus
			{
				AESTREAM * stream = ( ( __FLACAudioDecoder * ) client_data )->stream;
				int64_t readed = stream->read ( stream->object, buffer, *bytes );
				if ( readed == 0 )
					return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
				else if ( readed < 0 )
					return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
				*bytes = readed;

				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
			},
			[] ( const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data )
			-> FLAC__StreamDecoderSeekStatus
			{
				AESTREAM * stream = ( ( __FLACAudioDecoder * ) client_data )->stream;
				if ( stream->seek ( stream->object, absolute_byte_offset, AESO_BEGIN ) == -1 )
					return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
				return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
			},
			[] ( const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data )
			-> FLAC__StreamDecoderTellStatus
			{
				AESTREAM * stream = ( ( __FLACAudioDecoder * ) client_data )->stream;
				*absolute_byte_offset = stream->tell ( stream->object );
				if ( *absolute_byte_offset == -1 )
					return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
				return FLAC__STREAM_DECODER_TELL_STATUS_OK;
			},
			[] ( const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data )
			-> FLAC__StreamDecoderLengthStatus
			{
				AESTREAM * stream = ( ( __FLACAudioDecoder * ) client_data )->stream;
				*stream_length = stream->length ( stream->object );
				if ( *stream_length == -1 )
					return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
				return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
			},
			[] ( const FLAC__StreamDecoder *decoder, void *client_data ) -> FLAC__bool
			{
				AESTREAM * stream = ( ( __FLACAudioDecoder * ) client_data )->stream;
				return stream->tell ( stream->object ) == stream->length ( stream->object );
			},
			[] ( const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer [], void *client_data )
			-> FLAC__StreamDecoderWriteStatus
			{
				__FLACAudioDecoder * aedecoder = ( __FLACAudioDecoder * ) client_data;
				AESTREAM * stream = aedecoder->stream;

				int16_t * tempBuffer = ( int16_t * ) aedecoder->bytes;
				for ( unsigned i = 0; i < frame->header.blocksize; ++i )
				{
					for ( unsigned c = 0; c < frame->header.channels; ++c )
						WriteToBuffer16 ( tempBuffer++, ( FLAC__int16 ) buffer [ c ] [ i ] );
				}
				aedecoder->bytesRead = frame->header.blocksize * frame->header.channels * 2;
				return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
			},
			[] ( const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data )
			{
				__FLACAudioDecoder * aedecoder = ( __FLACAudioDecoder * ) client_data;
				aedecoder->totalSamples = ( int64_t ) metadata->data.stream_info.total_samples;
				aedecoder->samplerate = metadata->data.stream_info.sample_rate;
				aedecoder->channels = metadata->data.stream_info.channels;
				aedecoder->bitsPerSample = metadata->data.stream_info.bits_per_sample;

				aedecoder->byterate = ( aedecoder->channels * ( aedecoder->bitsPerSample / 8 ) * aedecoder->samplerate );
			},
			[] ( const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data )
			{

			}, this );

		if ( result != FLAC__STREAM_DECODER_INIT_STATUS_OK )
		{
			FLAC__stream_decoder_delete ( file );
			file = nullptr;
			return AEERROR_UNKNOWN;
		}

		FLAC__stream_decoder_process_until_end_of_metadata ( file );

		if ( channels == 0 )
			return AEERROR_FAIL;

		if ( !FLAC__stream_decoder_process_single ( file ) )
		{
			FLAC__stream_decoder_delete ( file );
			file = nullptr;
			return AEERROR_FAIL;
		}

		FLAC__stream_decoder_seek_absolute ( file, 0 );

		return AEERROR_NOERROR;
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format )
	{
		*format = AEWAVEFORMAT_initialize ( channels, bitsPerSample, samplerate, AEAF_PCM );
		return AEERROR_NOERROR;
	}
	error_t getDuration ( AETIMESPAN * timeSpan )
	{
		if ( isOggContainer )
			return AEERROR_NOT_SUPPORTED_FEATURE;
		uint64_t bytes = FLAC__stream_decoder_get_total_samples ( file );
		*timeSpan = AETIMESPAN_initializeWithByteCount ( bytes * channels * ( bitsPerSample / 8 ), byterate );
		return AEERROR_NOERROR;
	}
	error_t getReadPosition ( AETIMESPAN * timeSpan )
	{
		if ( isOggContainer )
			return AEERROR_NOT_SUPPORTED_FEATURE;
		uint64_t bytesPos;
		FLAC__stream_decoder_get_decode_position ( file, &bytesPos );
		*timeSpan = AETIMESPAN_initializeWithByteCount ( bytesPos, byterate );
		return AEERROR_NOERROR;
	}
	error_t setReadPosition ( AETIMESPAN timeSpan )
	{
		if ( isOggContainer )
			return AEERROR_NOT_SUPPORTED_FEATURE;
		return FLAC__stream_decoder_seek_absolute ( file, ( int64_t ) ( AETIMESPAN_totalSeconds ( timeSpan ) * byterate ) )
			? AEERROR_NOERROR : AEERROR_FAIL;
	}

public:
	error_t readSample ( AEAUDIOSAMPLE ** sample )
	{
		uint64_t bytesPos;
		if ( isOggContainer )
			bytesPos = 0;
		else 
			if ( !FLAC__stream_decoder_get_decode_position ( file, &bytesPos ) )
				return AEERROR_FAIL;
		AETIMESPAN current = AETIMESPAN_initializeWithByteCount ( bytesPos, byterate );

		bool loop = true;
		while ( loop )
		{
			if ( !FLAC__stream_decoder_process_single ( file ) )
				return AEERROR_FAIL;

			auto state = FLAC__stream_decoder_get_state ( file );
			switch ( state )
			{
				case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
					loop = false;
					break;
				case FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM:
					return AEERROR_END_OF_FILE;
				default:
					continue;
			}
		}

		std::shared_ptr<int8_t []> buffer ( new int8_t [ bytesRead ] );
		memcpy ( &buffer [ 0 ], ( void * ) bytes, bytesRead );

		AEAUDIOSAMPLE * ret = AE_allocInterfaceType ( AEAUDIOSAMPLE );
		ret->object = new __FLACAudioSample ( &buffer [ 0 ], bytesRead, current, AETIMESPAN_initializeWithSeconds ( bytesRead / ( float ) byterate ) );
		ret->free = [] ( void * obj ) { delete reinterpret_cast< __FLACAudioSample* >( obj ); };
		ret->tag = "AudEar Open Source Software CODEC FLAC and OggFLAC Audio Decoder's Sample";
		ret->getSampleTime = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __FLACAudioSample* >( obj )->getSampleTime ( timeSpan ); };
		ret->getSampleDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __FLACAudioSample* >( obj )->getSampleDuration ( timeSpan ); };
		ret->lock = [] ( void * obj, uint8_t ** buffer, int64_t * len ) { return reinterpret_cast< __FLACAudioSample* >( obj )->lock ( buffer, len ); };
		ret->unlock = [] ( void * obj ) { return reinterpret_cast< __FLACAudioSample* >( obj )->unlock (); };

		*sample = ret;

		return AEERROR_NOERROR;
	}

private:
	FLAC__StreamDecoder * file;
	AESTREAM * stream;

	int8_t bytes [ 1048576 ];
	int64_t bytesRead;

	int channels, bitsPerSample, samplerate, byterate;
	int64_t totalSamples;

	FLAC_INIT initFunc;
	bool isOggContainer;
};

error_t AE_createOggFLACAudioDecoder ( AEAUDIODECODER ** ret )
{
	AEAUDIODECODER * decoder = AE_allocInterfaceType ( AEAUDIODECODER );

	decoder->object = new __FLACAudioDecoder ( FLAC__stream_decoder_init_ogg_stream );
	decoder->free = [] ( void * obj ) { delete reinterpret_cast< __FLACAudioDecoder* >( obj ); };
	decoder->tag = "AudEar Open Source Software CODEC OggFLAC Audio Decoder";
	decoder->initialize = [] ( void * obj, AESTREAM * stream ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->initialize ( stream ); };
	decoder->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->getWaveFormat ( format ); };
	decoder->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->getDuration ( timeSpan ); };
	decoder->getReadPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->getReadPosition ( timeSpan ); };
	decoder->setReadPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->setReadPosition ( timeSpan ); };
	decoder->readSample = [] ( void * obj, AEAUDIOSAMPLE ** sample ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->readSample ( sample ); };

	*ret = decoder;

	return AEERROR_NOERROR;
}

error_t AE_createFLACAudioDecoder ( AEAUDIODECODER ** ret )
{
	AEAUDIODECODER * decoder = AE_allocInterfaceType ( AEAUDIODECODER );

	decoder->object = new __FLACAudioDecoder ( FLAC__stream_decoder_init_stream );
	decoder->free = [] ( void * obj ) { delete reinterpret_cast< __FLACAudioDecoder* >( obj ); };
	decoder->tag = "AudEar Open Source Software CODEC FLAC Audio Decoder";
	decoder->initialize = [] ( void * obj, AESTREAM * stream ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->initialize ( stream ); };
	decoder->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->getWaveFormat ( format ); };
	decoder->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->getDuration ( timeSpan ); };
	decoder->getReadPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->getReadPosition ( timeSpan ); };
	decoder->setReadPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->setReadPosition ( timeSpan ); };
	decoder->readSample = [] ( void * obj, AEAUDIOSAMPLE ** sample ) { return reinterpret_cast< __FLACAudioDecoder* >( obj )->readSample ( sample ); };

	*ret = decoder;

	return AEERROR_NOERROR;
}