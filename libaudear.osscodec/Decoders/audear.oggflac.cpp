#include "../audear.osscodec.h"

#define FLAC__NO_DLL
#include <FLAC/all.h>

#pragma comment ( lib, "libogg_static.lib" )
#pragma comment ( lib, "libFLAC_static.lib" )

void WriteToBuffer16 ( int16_t * buffer, FLAC__int16 data )
{
	union {
		int16_t word;
		int8_t arr [ 2 ];
	} temp;
	temp.arr [ 0 ] = ( int8_t ) ( data & 0xff );
	temp.arr [ 1 ] = ( int8_t ) ( ( data >> 8 ) & 0xff );
	*buffer = temp.word;
}

class AEInternalOggFLACSample : public AEBaseAudioSample
{
public:
	AEInternalOggFLACSample ( int8_t * bytes, int64_t length, AETimeSpan time, AETimeSpan duration )
		: bytes ( bytes ), length ( length ), time ( time ), duration ( duration )
	{ }

public:
	virtual error_t getSampleTime ( OUT AETimeSpan * time ) { *time = this->time; return AE_ERROR_SUCCESS; }
	virtual error_t getSampleDuration ( OUT AETimeSpan * time ) { *time = duration; return AE_ERROR_SUCCESS; }

public:
	virtual error_t lock ( OUT void ** buffer, OUT int64_t * length ) { *buffer = &bytes [ 0 ]; *length = this->length; return AE_ERROR_SUCCESS; }
	virtual error_t unlock () { return AE_ERROR_SUCCESS; }

private:
	std::shared_ptr<int8_t []> bytes;
	int64_t length;
	AETimeSpan time, duration;
};

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

class AEInternalOggFLACDecoder : public AEBaseAudioDecoder
{
public:
	AEInternalOggFLACDecoder ( FLAC_INIT initFunc )
		: file ( nullptr ), initFunc ( initFunc )
	{
		isOggContainer = ( initFunc == FLAC__stream_decoder_init_ogg_stream );
	}

	~AEInternalOggFLACDecoder ()
	{
		if ( file )
		{
			FLAC__stream_decoder_delete ( file );
			file = nullptr;
		}
	}

public:
	virtual error_t initialize ( IN AEBaseStream * stream )
	{
		if ( file )
		{
			FLAC__stream_decoder_delete ( file );
			file = nullptr;
		}

		stream->retain ();
		this->stream = stream;

		file = FLAC__stream_decoder_new ();
		auto result = initFunc ( file,
			[] ( const FLAC__StreamDecoder *decoder, FLAC__byte buffer [], size_t *bytes, void *client_data )
			-> FLAC__StreamDecoderReadStatus
		{
			error_t et;

			AEBaseStream * stream = ( ( AEInternalOggFLACDecoder * ) client_data )->stream;
			int64_t readed;
			if ( FAILED ( et = stream->read ( buffer, *bytes, &readed ) ) )
			{
				if ( et == AE_ERROR_END_OF_FILE ) return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
				else return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
			}
			*bytes = readed;

			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		},
			[] ( const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data )
			-> FLAC__StreamDecoderSeekStatus
		{
			AEBaseStream * stream = ( ( AEInternalOggFLACDecoder * ) client_data )->stream;
			if ( FAILED ( stream->seek ( kAESTREAMSEEK_SET, absolute_byte_offset, nullptr ) ) )
				return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
			return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
		},
			[] ( const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data )
			-> FLAC__StreamDecoderTellStatus
		{
			AEBaseStream * stream = ( ( AEInternalOggFLACDecoder * ) client_data )->stream;
			if ( FAILED ( stream->getPosition ( ( int64_t * ) absolute_byte_offset ) ) )
				return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
			return FLAC__STREAM_DECODER_TELL_STATUS_OK;
		},
			[] ( const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data )
			-> FLAC__StreamDecoderLengthStatus
		{
			AEBaseStream * stream = ( ( AEInternalOggFLACDecoder * ) client_data )->stream;
			if ( FAILED ( stream->getLength ( ( int64_t * ) stream_length ) ) )
				return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
			return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
		},
			[] ( const FLAC__StreamDecoder *decoder, void *client_data ) -> FLAC__bool
		{
			AEBaseStream * stream = ( ( AEInternalOggFLACDecoder * ) client_data )->stream;
			return false;
		},
			[] ( const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer [], void *client_data )
			-> FLAC__StreamDecoderWriteStatus
		{
			AEInternalOggFLACDecoder * aedecoder = ( AEInternalOggFLACDecoder * ) client_data;
			AEBaseStream * stream = aedecoder->stream;
			
			int16_t * tempBuffer = ( int16_t * ) aedecoder->bytes;
			for ( int i = 0; i < frame->header.blocksize; ++i )
			{
				for ( int c = 0; c < frame->header.channels; ++c )
					WriteToBuffer16 ( tempBuffer++, ( FLAC__int16 ) buffer [ c ] [ i ] );
			}
			aedecoder->bytesRead = frame->header.blocksize * frame->header.channels * 2;
			return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
		},
			[] ( const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data )
		{
			AEInternalOggFLACDecoder * aedecoder = ( AEInternalOggFLACDecoder * ) client_data;
			aedecoder->totalSamples = metadata->data.stream_info.total_samples;
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
			return AE_ERROR_UNKNOWN;
		}

		FLAC__stream_decoder_process_until_end_of_metadata ( file );

		/*channels = FLAC__stream_decoder_get_channels ( file );
		bitsPerSample = FLAC__stream_decoder_get_bits_per_sample ( file );
		samplerate = FLAC__stream_decoder_get_sample_rate ( file );

		byterate = ( channels * ( bitsPerSample / 8 ) * samplerate );*/
		if ( channels == 0 )
			return AE_ERROR_FAIL;

		if ( !FLAC__stream_decoder_process_single ( file ) )
		{
			FLAC__stream_decoder_delete ( file );
			file = nullptr;
			return AE_ERROR_FAIL;
		}

		FLAC__stream_decoder_seek_absolute ( file, 0 );

		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t getWaveFormat ( OUT AEWaveFormat * format )
	{
		*format = AEWaveFormat ( channels, bitsPerSample, samplerate, AE_WAVEFORMAT_PCM );
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getDuration ( OUT AETimeSpan * duration )
	{
		if ( isOggContainer )
		{
			*duration = AETimeSpan ();
			return AE_ERROR_NOT_SUPPORTED_FEATURE;
		}
		else
		{
			uint64_t bytes = FLAC__stream_decoder_get_total_samples ( file );
			*duration = AETimeSpan::fromByteCount ( bytes * channels * ( bitsPerSample / 8 ), byterate );
		}
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t setReadPosition ( AETimeSpan time )
	{
		if ( isOggContainer )
		{
			return AE_ERROR_NOT_SUPPORTED_FEATURE;
		}
		return FLAC__stream_decoder_seek_absolute ( file, time.getByteCount ( byterate ) ) ? AE_ERROR_SUCCESS : AE_ERROR_FAIL;
	}

public:
	virtual error_t getSample ( OUT AEBaseAudioSample ** sample )
	{
		uint64_t bytesPos;
		if ( isOggContainer )
			bytesPos = 0;
		else
		{
			if ( !FLAC__stream_decoder_get_decode_position ( file, &bytesPos ) )
				return AE_ERROR_FAIL;
		}
		AETimeSpan current = AETimeSpan::fromByteCount ( bytesPos, byterate );

		bool loop = true;
		while ( loop )
		{
			if ( !FLAC__stream_decoder_process_single ( file ) )
				return AE_ERROR_FAIL;

			auto state = FLAC__stream_decoder_get_state ( file );
			switch ( state )
			{
				case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
				//case FLAC__STREAM_DECODER_READ_FRAME:
					loop = false;
					break;
				case FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM:
					return AE_ERROR_END_OF_FILE;
				default:
					continue;
			}
		} 
		
		int8_t * buffer = new int8_t [ bytesRead ];
		memcpy ( buffer, ( void * ) bytes, bytesRead );

		*sample = new AEInternalOggFLACSample ( buffer, bytesRead, current, AETimeSpan::fromByteCount ( bytesRead, byterate ) );

		return AE_ERROR_SUCCESS;
	}

public:
	FLAC__StreamDecoder * file;
	AEBaseStream * stream;

	int8_t bytes [ 1048576 ];
	int64_t bytesRead;

	int channels, bitsPerSample, samplerate, totalSamples, byterate;

	FLAC_INIT initFunc;
	bool isOggContainer;
};

error_t AE_createOggFLACDecoder ( AEBaseAudioDecoder ** decoder )
{
	*decoder = new AEInternalOggFLACDecoder ( FLAC__stream_decoder_init_ogg_stream );
	return AE_ERROR_SUCCESS;
}

error_t AE_createFLACDecoder ( AEBaseAudioDecoder ** decoder )
{
	*decoder = new AEInternalOggFLACDecoder ( FLAC__stream_decoder_init_stream );
	return AE_ERROR_SUCCESS;
}