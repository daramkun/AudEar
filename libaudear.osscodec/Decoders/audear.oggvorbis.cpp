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
	virtual AEERROR getSampleTime ( OUT AETimeSpan * time ) { *time = this->time; return AEERROR_SUCCESS; }
	virtual AEERROR getSampleDuration ( OUT AETimeSpan * time ) { *time = duration; return AEERROR_SUCCESS; }

public:
	virtual AEERROR lock ( OUT void ** buffer, OUT int64_t * length ) { *buffer = &bytes [ 0 ]; *length = this->length; return AEERROR_SUCCESS; }
	virtual AEERROR unlock () { return AEERROR_SUCCESS; }

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
	virtual AEERROR initialize ( IN AEBaseStream * stream )
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
				case OV_EREAD: return AEERROR_INVALID_CALL;
				case OV_ENOTVORBIS: return AEERROR_INVALID_ARGUMENT;
				case OV_EVERSION: return AEERROR_NOT_SUPPORTED_FORMAT;
				case OV_EBADHEADER: return AEERROR_INVALID_ARGUMENT;
				case OV_EFAULT: return AEERROR_FAIL;
				default: return AEERROR_UNKNOWN;
			}
		}

		byterate = ( file.vi->channels * 2 * file.vi->rate );

		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR getWaveFormat ( OUT AEWaveFormat * format )
	{
		*format = AEWaveFormat ( file.vi->channels, 16, file.vi->rate, AE_WAVEFORMAT_PCM );
		return AEERROR_SUCCESS;
	}
	virtual AEERROR getDuration ( OUT AETimeSpan * duration )
	{
		*duration = AETimeSpan::fromSeconds ( ov_time_total ( &file, -1 ) );
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR setReadPosition ( AETimeSpan time )
	{
		ov_time_seek ( &file, time.totalSeconds () );
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR getSample ( OUT AEBaseAudioSample ** sample )
	{
		int length = byterate / 100;
		int8_t * buffer = new int8_t [ length ];
		int bitstream;
		AETimeSpan current = AETimeSpan::fromSeconds ( ov_time_tell ( &file ) );
		long result = ov_read ( &file, ( char * ) buffer, length, 0, 2, 1, &bitstream );

		if ( result == 0 )
		{
			delete [] buffer;
			return AEERROR_END_OF_FILE;
		}
		else if ( result == OV_HOLE || result == OV_EBADLINK || result == OV_EINVAL )
		{
			delete [] buffer;
			return AEERROR_FAIL;
		}

		*sample = new AEInternalOggVorbisSample ( buffer, result, current, AETimeSpan::fromByteCount ( result, byterate ) );

		return AEERROR_SUCCESS;
	}

private:
	OggVorbis_File file;
	int byterate;
};

AEERROR AE_createOggVorbisDecoder ( AEBaseAudioDecoder ** decoder )
{
	*decoder = new AEInternalOggVorbisDecoder ();
	return AEERROR_SUCCESS;
}