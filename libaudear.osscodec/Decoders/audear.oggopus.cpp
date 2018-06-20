#include "../audear.osscodec.h"

//#include <opus.h>
#include <opusfile.h>

#pragma comment ( lib, "libogg_static.lib" )
#pragma comment ( lib, "opus.lib" )
#pragma comment ( lib, "opusfile.lib" )

class AEInternalOggOpusSample : public AEBaseAudioSample
{
public:
	AEInternalOggOpusSample ( int8_t * bytes, int64_t length, AETimeSpan time, AETimeSpan duration )
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

class AEInternalOggOpusDecoder : public AEBaseAudioDecoder
{
public:
	AEInternalOggOpusDecoder ()
		: file ( nullptr ), channels ( 0 ), samplerate ( 0 )
	{
		callbacks.read = [] ( void *datasource, unsigned char *ptr, int nbytes ) -> int
		{
			AEBaseStream * stream = ( AEBaseStream* ) datasource;
			int64_t readed;
			if ( FAILED ( stream->read ( ptr, nbytes, &readed ) ) )
				return -1;
			return nbytes;
		};
		callbacks.seek = [] ( void *datasource, int64_t offset, int whence ) -> int
		{
			AEBaseStream * stream = ( AEBaseStream* ) datasource;

			bool canSeek;
			if ( FAILED ( stream->canSeek ( &canSeek ) ) || !canSeek )
				return -1;

			int64_t seeked;
			if ( FAILED ( stream->seek ( ( AESTREAMSEEK ) whence, offset, &seeked ) ) )
				return -1;

			return 0;
		};
		callbacks.tell = [] ( void *datasource ) -> int64_t
		{
			AEBaseStream * stream = ( AEBaseStream* ) datasource;
			int64_t pos;
			if ( FAILED ( stream->getPosition ( &pos ) ) )
				return -1;
			return ( long ) pos;
		};
		callbacks.close = [] ( void *datasource ) -> int
		{
			AEBaseStream * stream = ( AEBaseStream* ) datasource;
			stream->release ();
			return 0;
		};
	}

	~AEInternalOggOpusDecoder ()
	{
		if ( file )
			op_free ( file );
	}

public:
	virtual AEERROR initialize ( IN AEBaseStream * stream )
	{
		if ( file )
		{
			op_free ( file );
		}

		stream->retain ();

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

		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR getWaveFormat ( OUT AEWaveFormat * format )
	{
		*format = AEWaveFormat ( channels, 16, samplerate, AE_WAVEFORMAT_PCM );
		return AEERROR_SUCCESS;
	}
	virtual AEERROR getDuration ( OUT AETimeSpan * duration )
	{
		*duration = AETimeSpan::fromByteCount ( op_pcm_total ( file, -1 ), samplerate );
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR setReadPosition ( AETimeSpan time )
	{
		int64_t seekOffset = time.getByteCount ( samplerate );
		op_pcm_seek ( file, seekOffset );
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR getSample ( OUT AEBaseAudioSample ** sample )
	{
		int length = byterate / 100;
		int8_t * buffer = new int8_t [ byterate ];
		int bitstream;
		AETimeSpan current = AETimeSpan::fromByteCount ( op_pcm_tell ( file ), samplerate );
		long result = op_read ( file, ( int16_t * ) buffer, length / 2, &bitstream );

		if ( result == 0 )
		{
			delete [] buffer;
			return AEERROR_END_OF_FILE;
		}
		else if ( result == OP_HOLE || result == OP_EBADLINK || result == OP_EINVAL )
		{
			delete [] buffer;
			return AEERROR_FAIL;
		}

		result *= ( channels * 2 );
		*sample = new AEInternalOggOpusSample ( buffer, result, current, AETimeSpan::fromByteCount ( result, byterate ) );

		return AEERROR_SUCCESS;
	}

private:
	OggOpusFile * file;

	int channels, samplerate;
	int byterate;

	OpusFileCallbacks callbacks;
};

AEERROR AE_createOggOpusDecoder ( AEBaseAudioDecoder ** decoder )
{
	*decoder = new AEInternalOggOpusDecoder ();
	return AEERROR_SUCCESS;
}