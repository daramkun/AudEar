#include "../audear.osscodec.h"

#include <mp4ff/mp4ff.h>
#include <faad2/faad.h>
#include <faad2/neaacdec.h>

#pragma comment ( lib, "libfaad.lib" )
#pragma comment ( lib, "mp4ff.lib" )

class AEInternalMP4AACSample : public AEBaseAudioSample
{
public:
	AEInternalMP4AACSample ( uint8_t * bytes, int64_t length, AETimeSpan time, AETimeSpan duration )
		: bytes ( bytes ), length ( length ), time ( time ), duration ( duration )
	{ }

public:
	virtual AEERROR getSampleTime ( OUT AETimeSpan * time ) { *time = this->time; return AEERROR_SUCCESS; }
	virtual AEERROR getSampleDuration ( OUT AETimeSpan * time ) { *time = duration; return AEERROR_SUCCESS; }

public:
	virtual AEERROR lock ( OUT void ** buffer, OUT int64_t * length ) { *buffer = &bytes [ 0 ]; *length = this->length; return AEERROR_SUCCESS; }
	virtual AEERROR unlock () { return AEERROR_SUCCESS; }

private:
	std::shared_ptr<uint8_t []> bytes;
	int64_t length;
	AETimeSpan time, duration;
};

class AEInternalMP4AACDecoder : public AEBaseAudioDecoder
{
public:
	AEInternalMP4AACDecoder ()
		: mp4ff ( nullptr ), aacHandle ( 0 )
	{
		callback.read = [] ( void *user_data, void *buffer, uint32_t length ) -> uint32_t
		{
			AEBaseStream * stream = ( AEBaseStream * ) user_data;
			int64_t readed;
			if ( FAILED ( stream->read ( buffer, length, &readed ) ) )
				return 0;
			return readed;
		};
		callback.seek = [] ( void *user_data, uint64_t position ) -> uint32_t
		{
			AEBaseStream * stream = ( AEBaseStream * ) user_data;
			int64_t seeked;
			if ( FAILED ( stream->seek ( kAESTREAMSEEK_SET, position, &seeked ) ) )
				return -1;
			return seeked;
		};
		callback.write = [] ( void *udata, void *buffer, uint32_t length ) -> uint32_t { return 0; };
		callback.truncate = [] ( void *user_data ) -> uint32_t { return -1; };
	}

	~AEInternalMP4AACDecoder ()
	{
		mp4ff_close ( mp4ff );
	}

public:
	virtual AEERROR initialize ( IN AEBaseStream * stream )
	{
		if ( mp4ff != 0 )
		{
			( ( AEBaseStream * ) callback.user_data )->release ();
			mp4ff_close ( mp4ff );
		}
		if ( aacHandle )
		{
			NeAACDecClose ( aacHandle );
			aacHandle = 0;
		}

		stream->retain ();
		callback.user_data = stream;

		mp4ff = mp4ff_open_read ( &callback );

		if ( mp4ff == nullptr )
		{
			( ( AEBaseStream * ) callback.user_data )->release ();
			callback.user_data = nullptr;
			return AEERROR_UNKNOWN;
		}

		sampleIndex = 0;

		channels = mp4ff_get_channel_count ( mp4ff, 0 );
		samplerate = mp4ff_get_sample_rate ( mp4ff, 0 );

		byterate = ( channels * 2 * samplerate );

		aacHandle = NeAACDecOpen ();

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
		int64_t du = mp4ff_get_track_duration ( mp4ff, 0 );
		*duration = AETimeSpan::fromSeconds ( ( double ) du );
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR setReadPosition ( AETimeSpan time )
	{
		sampleIndex = ( int ) round ( time.totalSeconds () / 0.026125 );
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR getSample ( OUT AEBaseAudioSample ** sample )
	{
		uint8_t * buffer = new uint8_t [ 48000 ];
		int bitstream;

		int length;
		if ( ( length = mp4ff_read_sample_v2 ( mp4ff, 0, sampleIndex++, buffer ) ) <= 0 )
			return AEERROR_END_OF_FILE;

		NeAACDecInit ( aacHandle, buffer, length, nullptr, nullptr );

		delete [] buffer;

		uint8_t * outputBuffer = new uint8_t [ 48000 ];

		NeAACDecFrameInfo frameInfo;
		NeAACDecDecode ( aacHandle, &frameInfo, outputBuffer, 48000 );

		*sample = new AEInternalMP4AACSample ( outputBuffer, frameInfo.bytesconsumed, AETimeSpan (), AETimeSpan::fromByteCount ( frameInfo.bytesconsumed, byterate ) );

		return AEERROR_SUCCESS;
	}

private:
	mp4ff_t * mp4ff;
	int sampleIndex;

	int channels, samplerate, byterate;

	mp4ff_callback_t callback;


	NeAACDecHandle aacHandle;
};

EXTC AEERROR AEOSSCEXP AE_createM4AAACDecoder ( AEBaseAudioDecoder ** decoder )
{
	*decoder = new AEInternalMP4AACDecoder ();
	return AEERROR_SUCCESS;
}