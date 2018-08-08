#include "../audear.osscodec.h"

#define DR_WAV_IMPLEMENTATION
#include <dr_wav/dr_wav.h>

#include <memory>

class __DrWAVAudioSample
{
public:
	__DrWAVAudioSample ( int8_t * bytes, int64_t length, AETIMESPAN time, AETIMESPAN duration )
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

class __DrWAVAudioDecoder
{
public:
	__DrWAVAudioDecoder ()
		: file ( nullptr )
	{

	}
	~__DrWAVAudioDecoder ()
	{
		if ( file != nullptr )
			drwav_close ( file );
		file = nullptr;
	}

public:
	error_t initialize ( AESTREAM * stream )
	{
		if ( file != nullptr )
		{
			drwav_close ( file );
			file = nullptr;
		}

		AE_retainInterface ( stream );

		file = drwav_open ( 
			[] ( void* pUserData, void* pBufferOut, size_t bytesToRead ) -> size_t
			{
				AESTREAM * stream = ( AESTREAM* ) pUserData;
				int64_t readed = stream->read ( stream->object, ( uint8_t* ) pBufferOut, bytesToRead );
				return ( size_t ) ( readed );
			},
			[] ( void* pUserData, int offset, drwav_seek_origin origin ) -> drwav_bool32
			{
				AESTREAM * stream = ( AESTREAM* ) pUserData;
				return stream->seek ( stream->object, offset, ( AESEEKORIGIN ) origin ) >= 0;
			}, stream );

		if ( file == nullptr )
			return AEERROR_NOT_SUPPORTED_FORMAT;

		fixedBps = file->bitsPerSample;
		if ( fixedBps > 16 )
			fixedBps = 16;
		else if ( fixedBps < 16 )
			fixedBps = 32;

		byterate = ( file->channels * ( fixedBps / 8 ) * file->sampleRate );

		return AEERROR_NOERROR;
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format )
	{
		if ( file == nullptr )
			return AEERROR_INVALID_CALL;
		*format = AEWAVEFORMAT_initialize ( file->channels, fixedBps, file->sampleRate, AEAF_PCM );
		return AEERROR_NOERROR;
	}
	error_t getDuration ( AETIMESPAN * timeSpan )
	{
		if ( file == nullptr )
			return AEERROR_INVALID_CALL;
		*timeSpan = AETIMESPAN_initializeWithByteCount ( ( ( file->totalSampleCount + 1 ) * ( fixedBps / 8 ) ), byterate );
		return AEERROR_NOERROR;
	}
	error_t getReadPosition ( AETIMESPAN * timeSpan )
	{
		if ( file == nullptr )
			return AEERROR_INVALID_CALL;
		*timeSpan = AETIMESPAN_initializeWithByteCount ( file->dataChunkDataSize - file->bytesRemaining, byterate );
		return AEERROR_NOERROR;
	}
	error_t setReadPosition ( AETIMESPAN timeSpan )
	{
		if ( file == nullptr )
			return AEERROR_INVALID_CALL;

		drwav_seek_to_sample ( file, ( int64_t ) ( AETIMESPAN_totalSeconds ( timeSpan ) * byterate ) / ( fixedBps / 8 ) );
		return AEERROR_NOERROR;
	}

public:
	error_t readSample ( AEAUDIOSAMPLE ** sample )
	{
		AEAUDIOBUFFER<int8_t> buffer ( byterate );
		AETIMESPAN current = AETIMESPAN_initializeWithByteCount ( file->dataChunkDataSize - file->bytesRemaining, byterate );
		drwav_uint64 result;
		if ( file->bitsPerSample == 32 )
			result = drwav_read_s32 ( file, byterate / 4, ( drwav_int32 * ) &buffer [ 0 ] );
		else if ( file->bitsPerSample == 16 )
			result = drwav_read_s16 ( file, byterate / 2, ( drwav_int16 * ) &buffer [ 0 ] );

		if ( result == 0 )
			return AEERROR_END_OF_FILE;
		else if ( result , 0 )
			return AEERROR_FAIL;

		AEAUDIOSAMPLE * ret = AE_allocInterfaceType ( AEAUDIOSAMPLE );
		ret->object = new __DrWAVAudioSample ( buffer, result * ( fixedBps / 8 ), current, AETIMESPAN_initializeWithSeconds ( result / ( float ) byterate ) );
		ret->free = [] ( void * obj ) { delete reinterpret_cast< __DrWAVAudioSample* >( obj ); };
		ret->tag = "AudEar Open Source Software CODEC Dr WAV's WAV Audio Decoder's Sample";
		ret->getSampleTime = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __DrWAVAudioSample* >( obj )->getSampleTime ( timeSpan ); };
		ret->getSampleDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __DrWAVAudioSample* >( obj )->getSampleDuration ( timeSpan ); };
		ret->lock = [] ( void * obj, uint8_t ** buffer, int64_t * len ) { return reinterpret_cast< __DrWAVAudioSample* >( obj )->lock ( buffer, len ); };
		ret->unlock = [] ( void * obj ) { return reinterpret_cast< __DrWAVAudioSample* >( obj )->unlock (); };

		*sample = ret;

		return AEERROR_NOERROR;
	}

private:
	drwav * file;
	int byterate, fixedBps;
};

error_t AE_createWAVAudioDecoder ( AEAUDIODECODER ** ret )
{
	AEAUDIODECODER * decoder = AE_allocInterfaceType ( AEAUDIODECODER );

	decoder->object = new __DrWAVAudioDecoder ();
	decoder->free = [] ( void * obj ) { delete reinterpret_cast< __DrWAVAudioDecoder* >( obj ); };
	decoder->tag = "AudEar Open Source Software CODEC DrWAV's WAV Audio Decoder";
	decoder->initialize = [] ( void * obj, AESTREAM * stream ) { return reinterpret_cast< __DrWAVAudioDecoder* >( obj )->initialize ( stream ); };
	decoder->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __DrWAVAudioDecoder* >( obj )->getWaveFormat ( format ); };
	decoder->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __DrWAVAudioDecoder* >( obj )->getDuration ( timeSpan ); };
	decoder->getReadPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __DrWAVAudioDecoder* >( obj )->getReadPosition ( timeSpan ); };
	decoder->setReadPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __DrWAVAudioDecoder* >( obj )->setReadPosition ( timeSpan ); };
	decoder->readSample = [] ( void * obj, AEAUDIOSAMPLE ** sample ) { return reinterpret_cast< __DrWAVAudioDecoder* >( obj )->readSample ( sample ); };

	*ret = decoder;

	return AEERROR_NOERROR;
}