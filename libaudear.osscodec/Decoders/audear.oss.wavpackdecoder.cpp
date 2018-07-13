#include "../audear.osscodec.h"

#include <memory>

/*#include <wavpack/wavpack.h>
#pragma comment ( lib, "libwavpack.lib" )

class __WavPackAudioSample
{
public:
	__WavPackAudioSample ( int8_t * bytes, int64_t length, AETIMESPAN time, AETIMESPAN duration )
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

class __WavPackAudioDecoder
{
public:
	__WavPackAudioDecoder ()
	{
		reader.read_bytes = [] ( void * id, void * bytes, int32_t bcount ) -> int32_t
		{
			AESTREAM * stream = ( AESTREAM* ) id;
			return ( int32_t ) stream->read ( stream->object, ( uint8_t* ) bytes, bcount );
		};
		reader.write_bytes = nullptr;
		reader.get_pos = [] ( void * id ) -> int64_t 
		{
			AESTREAM * stream = ( AESTREAM* ) id;
			return stream->tell ( stream->object );
		};
		reader.set_pos_abs = [] ( void * id, int64_t pos ) -> int
		{
			AESTREAM * stream = ( AESTREAM* ) id;
			return stream->seek ( stream->object, pos, AESO_BEGIN );
		};
		reader.set_pos_rel = [] ( void * id, int64_t delta, int mode ) -> int
		{
			AESTREAM * stream = ( AESTREAM* ) id;
			return stream->seek ( stream->object, delta, AESO_CURRENT );
		};
		reader.push_back_byte = nullptr;
		reader.get_length = [] ( void * id ) -> int64_t
		{
			AESTREAM * stream = ( AESTREAM* ) id;
			return stream->length ( stream->object );
		};
		reader.can_seek = [] ( void * id ) -> int
		{
			AESTREAM * stream = ( AESTREAM* ) id;
			return stream->seek ( stream->object, 0, AESO_CURRENT ) == 0;
		};
		reader.truncate_here = nullptr;
		reader.close = [] ( void * id ) -> int
		{
			AESTREAM * stream = ( AESTREAM* ) id;
			AE_releaseInterface ( ( void** ) stream );
		};
	}
	~__WavPackAudioDecoder ()
	{
		if ( context != nullptr )
			context = WavpackCloseFile ( context );
	}

public:
	error_t initialize ( AESTREAM * stream )
	{
		if ( context != nullptr )
			context = WavpackCloseFile ( context );

		AE_retainInterface ( stream );

		context = WavpackOpenFileInputEx64 ( &reader, stream, nullptr, nullptr, 0, 0 );

		if ( context == nullptr )
		{
			AE_releaseInterface ( ( void ** ) &stream );
			return AEERROR_NOT_SUPPORTED_FORMAT;
		}

		channels = WavpackGetNumChannels ( context );
		bitsPerSample = WavpackGetBitsPerSample ( context );
		samplerate = WavpackGetSampleRate ( context );
		byterate = ( channels * ( bitsPerSample / 8 ) * samplerate );

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
		*timeSpan = AETIMESPAN_initializeWithByteCount ( WavpackGetNumSamples64 ( context ) * byterate, byterate );
		return AEERROR_NOERROR;
	}
	error_t getReadPosition ( AETIMESPAN * timeSpan )
	{
		*timeSpan = AETIMESPAN_initializeWithSeconds ( ov_time_tell ( &file ) );
		return AEERROR_NOERROR;
	}
	error_t setReadPosition ( AETIMESPAN timeSpan )
	{
		ov_time_seek ( &file, AETIMESPAN_totalSeconds ( timeSpan ) );
		return AEERROR_NOERROR;
	}

public:
	error_t readSample ( AEAUDIOSAMPLE ** sample )
	{
		int length = byterate / 100;
		std::shared_ptr<int8_t []> buffer ( new int8_t [ length ] );
		int bitstream;
		AETIMESPAN current = AETIMESPAN_initializeWithSeconds ( ov_time_tell ( &file ) );
		long result = ov_read ( &file, ( char * ) &buffer [ 0 ], length, 0, 2, 1, &bitstream );
		

		if ( result == 0 )
			return AEERROR_END_OF_FILE;
		else if ( result == OV_HOLE || result == OV_EBADLINK || result == OV_EINVAL )
			return AEERROR_FAIL;

		AEAUDIOSAMPLE * ret = AE_allocInterfaceType ( AEAUDIOSAMPLE );
		ret->object = new __WavPackAudioSample ( &buffer [ 0 ], result, current, AETIMESPAN_initializeWithSeconds ( result / ( float ) byterate ) );
		ret->free = [] ( void * obj ) { delete reinterpret_cast< __WavPackAudioSample* >( obj ); };
		ret->tag = "AudEar Open Source Software CODEC OggVorbis Audio Decoder's Sample";
		ret->getSampleTime = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __WavPackAudioSample* >( obj )->getSampleTime ( timeSpan ); };
		ret->getSampleDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __WavPackAudioSample* >( obj )->getSampleDuration ( timeSpan ); };
		ret->lock = [] ( void * obj, uint8_t ** buffer, int64_t * len ) { return reinterpret_cast< __WavPackAudioSample* >( obj )->lock ( buffer, len ); };
		ret->unlock = [] ( void * obj ) { return reinterpret_cast< __WavPackAudioSample* >( obj )->unlock (); };

		*sample = ret;

		return AEERROR_NOERROR;
	}

private:
	WavpackStreamReader64 reader;
	WavpackContext * context;
	int channels, bitsPerSample, samplerate, byterate;
};

error_t AE_createWavPackAudioDecoder ( AEAUDIODECODER ** ret )
{
	AEAUDIODECODER * decoder = AE_allocInterfaceType ( AEAUDIODECODER );

	decoder->object = new __WavPackAudioDecoder ();
	decoder->free = [] ( void * obj ) { delete reinterpret_cast< __WavPackAudioDecoder* >( obj ); };
	decoder->tag = "AudEar Open Source Software CODEC WavPack Audio Decoder";
	decoder->initialize = [] ( void * obj, AESTREAM * stream ) { return reinterpret_cast< __WavPackAudioDecoder* >( obj )->initialize ( stream ); };
	decoder->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __WavPackAudioDecoder* >( obj )->getWaveFormat ( format ); };
	decoder->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __WavPackAudioDecoder* >( obj )->getDuration ( timeSpan ); };
	decoder->getReadPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __WavPackAudioDecoder* >( obj )->getReadPosition ( timeSpan ); };
	decoder->setReadPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __WavPackAudioDecoder* >( obj )->setReadPosition ( timeSpan ); };
	decoder->readSample = [] ( void * obj, AEAUDIOSAMPLE ** sample ) { return reinterpret_cast< __WavPackAudioDecoder* >( obj )->readSample ( sample ); };

	*ret = decoder;

	return AEERROR_NOERROR;
}*/