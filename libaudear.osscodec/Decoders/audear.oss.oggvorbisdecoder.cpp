#include "../audear.osscodec.h"

#include <memory>

#include <vorbis/vorbisfile.h>
#pragma comment ( lib, "libogg_static.lib" )
#pragma comment ( lib, "libvorbis_static.lib" )
#pragma comment ( lib, "libvorbisfile_static.lib" )

class __OggVorbisAudioSample
{
public:
	__OggVorbisAudioSample ( int8_t * bytes, int64_t length, AETIMESPAN time, AETIMESPAN duration )
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

class __OggVorbisAudioDecoder
{
public:
	__OggVorbisAudioDecoder ()
	{
		memset ( &file, 0, sizeof ( OggVorbis_File ) );
	}
	~__OggVorbisAudioDecoder ()
	{
		ov_clear ( &file );
	}

public:
	error_t initialize ( AESTREAM * stream )
	{
		if ( file.datasource != nullptr )
		{
			ov_clear ( &file );
		}

		memset ( &file, 0, sizeof ( OggVorbis_File ) );
		AE_retainInterface ( stream );

		int result = ov_open_callbacks ( stream, &file, nullptr, 0,
		{
			[] ( void *ptr, size_t size, size_t nmemb, void *datasource ) -> size_t
			{
				AESTREAM * stream = ( AESTREAM* ) datasource;
				int64_t readed = stream->read ( stream->object, ( uint8_t* ) ptr, size * nmemb );
				return ( size_t ) ( readed / size );
			},
			[] ( void *datasource, ogg_int64_t offset, int whence ) -> int
			{
				AESTREAM * stream = ( AESTREAM* ) datasource;
				return ( int ) stream->seek ( stream->object, offset, ( AESEEKORIGIN ) whence );
			},
			[] ( void *datasource ) -> int
			{
				AESTREAM * stream = ( AESTREAM* ) datasource;
				AE_releaseInterface ( ( void ** ) &stream );
				return 0;
			},
			[] ( void *datasource ) -> long
			{
				AESTREAM * stream = ( AESTREAM* ) datasource;
				return ( long ) stream->tell ( stream->object );
			}
		} );

		if ( result != 0 )
		{
			ov_clear ( &file );
			switch ( result )
			{
				case OV_EREAD: return AEERROR_INVALID_CALL;
				case OV_ENOTVORBIS: return AEERROR_INVALID_ARGUMENT;
				case OV_EVERSION: return AEERROR_NOT_SUPPORTED_FORMAT;
				case OV_EBADHEADER: return AEERROR_INVALID_ARGUMENT;
				case OV_EFAULT: return AEERROR_FAIL;
				default: return AEERROR_UNKNOWN;
			}
		}

		byterate = ( file.vi->channels * 2 * file.vi->rate );

		return AEERROR_NOERROR;
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format )
	{
		*format = AEWAVEFORMAT_initialize ( file.vi->channels, 16, file.vi->rate, AEAF_PCM );
		return AEERROR_NOERROR;
	}
	error_t getDuration ( AETIMESPAN * timeSpan )
	{
		*timeSpan = AETIMESPAN_initializeWithSeconds ( ov_time_total ( &file, -1 ) );
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
		ret->object = new __OggVorbisAudioSample ( &buffer [ 0 ], result, current, AETIMESPAN_initializeWithSeconds ( result / ( float ) byterate ) );
		ret->free = [] ( void * obj ) { delete reinterpret_cast< __OggVorbisAudioSample* >( obj ); };
		ret->tag = "AudEar Open Source Software CODEC OggVorbis Audio Decoder's Sample";
		ret->getSampleTime = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OggVorbisAudioSample* >( obj )->getSampleTime ( timeSpan ); };
		ret->getSampleDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OggVorbisAudioSample* >( obj )->getSampleDuration ( timeSpan ); };
		ret->lock = [] ( void * obj, uint8_t ** buffer, int64_t * len ) { return reinterpret_cast< __OggVorbisAudioSample* >( obj )->lock ( buffer, len ); };
		ret->unlock = [] ( void * obj ) { return reinterpret_cast< __OggVorbisAudioSample* >( obj )->unlock (); };

		*sample = ret;

		return AEERROR_NOERROR;
	}

private:
	OggVorbis_File file;
	int byterate;
};

error_t AE_createOggVorbisAudioDecoder ( AEAUDIODECODER ** ret )
{
	AEAUDIODECODER * decoder = AE_allocInterfaceType ( AEAUDIODECODER );

	decoder->object = new __OggVorbisAudioDecoder ();
	decoder->free = [] ( void * obj ) { delete reinterpret_cast< __OggVorbisAudioDecoder* >( obj ); };
	decoder->tag = "AudEar Open Source Software CODEC OggVorbis Audio Decoder";
	decoder->initialize = [] ( void * obj, AESTREAM * stream ) { return reinterpret_cast< __OggVorbisAudioDecoder* >( obj )->initialize ( stream ); };
	decoder->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __OggVorbisAudioDecoder* >( obj )->getWaveFormat ( format ); };
	decoder->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OggVorbisAudioDecoder* >( obj )->getDuration ( timeSpan ); };
	decoder->getReadPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __OggVorbisAudioDecoder* >( obj )->getReadPosition ( timeSpan ); };
	decoder->setReadPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __OggVorbisAudioDecoder* >( obj )->setReadPosition ( timeSpan ); };
	decoder->readSample = [] ( void * obj, AEAUDIOSAMPLE ** sample ) { return reinterpret_cast< __OggVorbisAudioDecoder* >( obj )->readSample ( sample ); };

	*ret = decoder;

	return AEERROR_NOERROR;
}