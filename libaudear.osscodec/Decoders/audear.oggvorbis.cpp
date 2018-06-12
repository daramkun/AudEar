#include "../audear.osscodec.h"

#include <vorbis/vorbisfile.h>
#pragma comment ( lib, "libogg_static.lib" )
#pragma comment ( lib, "libvorbis_static.lib" )
#pragma comment ( lib, "libvorbisfile_static.lib" )

class AEInternalOggVorbisSample : public AEBaseAudioSample
{
public:
	AEInternalOggVorbisSample ( int8_t * bytes, int64_t length, AETimeSpan time, AETimeSpan duration )
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

class AEInternalOggVorbisDecoder : public AEBaseAudioDecoder
{
public:
	AEInternalOggVorbisDecoder ()
	{
		memset ( &file, 0, sizeof ( OggVorbis_File ) );
	}

	~AEInternalOggVorbisDecoder ()
	{
		ov_clear ( &file );
	}

public:
	virtual error_t initialize ( IN AEBaseStream * stream )
	{
		if ( file.datasource != nullptr )
		{
			ov_clear ( &file );
		}

		memset ( &file, 0, sizeof ( OggVorbis_File ) );
		stream->retain ();

		int result = ov_open_callbacks ( stream, &file, nullptr, 0,
		{
			[] ( void *ptr, size_t size, size_t nmemb, void *datasource ) -> size_t
			{
				AEBaseStream * stream = ( AEBaseStream* ) datasource;
				int64_t readed;
				stream->read ( ptr, size * nmemb, &readed );
				return ( size_t ) ( readed / size );
			},
			[] ( void *datasource, ogg_int64_t offset, int whence ) -> int
			{
				AEBaseStream * stream = ( AEBaseStream* ) datasource;

				bool canSeek;
				if ( FAILED ( stream->canSeek ( &canSeek ) ) || !canSeek )
					return -1;

				int64_t seeked;
				if ( FAILED ( stream->seek ( ( AESTREAMSEEK ) whence, offset, &seeked ) ) )
					return -1;

				return ( int ) seeked;
			},
			[] ( void *datasource ) -> int
			{
				AEBaseStream * stream = ( AEBaseStream* ) datasource;
				stream->release ();
				return 0;
			},
			[] ( void *datasource ) -> long
			{
				AEBaseStream * stream = ( AEBaseStream* ) datasource;
				int64_t pos;
				if ( FAILED ( stream->getPosition ( &pos ) ) )
					return -1;
				return ( long ) pos;
			}
		} );

		if ( result != 0 )
		{
			ov_clear ( &file );
			switch ( result )
			{
				case OV_EREAD: return AE_ERROR_INVALID_CALL;
				case OV_ENOTVORBIS: return AE_ERROR_INVALID_ARGUMENT;
				case OV_EVERSION: return AE_ERROR_NOT_SUPPORTED_FORMAT;
				case OV_EBADHEADER: return AE_ERROR_INVALID_ARGUMENT;
				case OV_EFAULT: return AE_ERROR_FAIL;
				default: return AE_ERROR_UNKNOWN;
			}
		}

		byterate = ( file.vi->channels * 2 * file.vi->rate );

		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t getWaveFormat ( OUT AEWaveFormat * format )
	{
		*format = AEWaveFormat ( file.vi->channels, 16, file.vi->rate, AE_WAVEFORMAT_PCM );
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getDuration ( OUT AETimeSpan * duration )
	{
		*duration = AETimeSpan::fromSeconds ( ov_time_total ( &file, -1 ) );
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t setReadPosition ( AETimeSpan time )
	{
		ov_time_seek ( &file, time.totalSeconds () );
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t getSample ( OUT AEBaseAudioSample ** sample )
	{
		int length = byterate / 100;
		int8_t * buffer = new int8_t [ length ];
		int bitstream;
		AETimeSpan current = AETimeSpan::fromSeconds ( ov_time_tell ( &file ) );
		long result = ov_read ( &file, ( char * ) buffer, length, 0, 2, 1, &bitstream );

		if ( result == 0 )
		{
			delete [] buffer;
			return AE_ERROR_END_OF_FILE;
		}
		else if ( result == OV_HOLE || result == OV_EBADLINK || result == OV_EINVAL )
		{
			delete [] buffer;
			return AE_ERROR_FAIL;
		}

		*sample = new AEInternalOggVorbisSample ( buffer, result, current, AETimeSpan::fromByteCount ( result, byterate ) );

		return AE_ERROR_SUCCESS;
	}

private:
	OggVorbis_File file;
	int byterate;
};

error_t AE_createOggVorbisDecoder ( AEBaseAudioDecoder ** decoder )
{
	*decoder = new AEInternalOggVorbisDecoder ();
	return AE_ERROR_SUCCESS;
}