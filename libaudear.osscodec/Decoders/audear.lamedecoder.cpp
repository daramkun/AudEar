#include "../audear.osscodec.h"

#include "../Utility/audear.utility.h"

#include <lame/lame.h>
#pragma comment ( lib, "libmp3lame-static.lib" )
#pragma comment ( lib, "libmpghip-static.lib" )

class AEInternalLameSample : public AEBaseAudioSample
{
public:
	AEInternalLameSample ( int16_t * bytes, int64_t length, AETimeSpan time, AETimeSpan duration )
		: bytes ( new int8_t [ length ] ), length ( length ), time ( time ), duration ( duration )
	{
		memcpy ( &this->bytes [ 0 ], bytes, length );
	}

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

class AEInternalLameDecoder : public AEBaseAudioDecoder
{
public:
	AEInternalLameDecoder ()
		: hip ( 0 ), stream ( nullptr )
	{

	}

	~AEInternalLameDecoder ()
	{
		if ( hip != 0 )
		{
			hip_decode_exit ( hip );
			hip = 0;
		}
	}

public:
	virtual AEERROR initialize ( IN AEBaseStream * stream )
	{
		cache_chunk_position ( stream, posizeCache, &channels, &samplerate );

		if ( posizeCache.size () == 0 )
			return AEERROR_NOT_SUPPORTED_FORMAT;

		if ( FAILED ( stream->seek ( kAESTREAMSEEK_SET, 0, nullptr ) ) )
			return AEERROR_FAIL;

		this->stream.release ();
		if ( hip != 0 )
			hip_decode_exit ( hip );

		this->stream = stream;

		hip = hip_decode_init ();

		byterate = channels * 2 * samplerate;

		nextIndex = 0;

		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR getWaveFormat ( OUT AEWaveFormat * format )
	{
		*format = AEWaveFormat ( channels, 16, samplerate );
		return AEERROR_SUCCESS;
	}
	virtual AEERROR getDuration ( OUT AETimeSpan * duration )
	{
		*duration = AETimeSpan::fromSeconds ( posizeCache.size () * 0.026125 );
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR setReadPosition ( AETimeSpan time )
	{
		nextIndex = ( int ) round ( time.totalSeconds () / 0.026125 );
		auto posize = posizeCache.at ( nextIndex++ );
		if ( FAILED ( stream->seek ( kAESTREAMSEEK_SET, posize.position, nullptr ) ) )
			return AEERROR_FAIL;
		return AEERROR_SUCCESS;
	}

public:
	virtual AEERROR getSample ( OUT AEBaseAudioSample ** sample )
	{
		uint8_t buffer [ 48000 ];
		int16_t lrPCM [ 2 ] [ 48000 ];
		int16_t mixedPCM [ 48000 * 2 ];
		int64_t readed;

		if ( nextIndex >= posizeCache.size () )
			return AEERROR_END_OF_FILE;

		auto posize = posizeCache.at ( nextIndex++ );
		
		int64_t pos;
		if ( FAILED ( stream->getPosition ( &pos ) ) )
			return AEERROR_FAIL;

		if ( pos != posize.position )
			if ( FAILED ( stream->seek ( kAESTREAMSEEK_SET, posize.position, nullptr ) ) )
				return AEERROR_FAIL;

		if ( FAILED ( stream->read ( buffer, posize.size, &readed ) ) )
			return AEERROR_FAIL;

		int result = hip_decode1 ( hip, buffer, 0, lrPCM [ 0 ], lrPCM [ 1 ] );
		if ( result == 0 )
		{
			result = hip_decode1 ( hip, buffer, min ( posize.size, readed ), lrPCM [ 0 ], lrPCM [ 1 ] );
			if ( result == 0 )
				return AEERROR_FAIL;
		}

		for ( int j = 0; j < result; ++j )
		{
			if ( channels == 2 )
			{
				mixedPCM [ j * 2 ] = lrPCM [ 0 ] [ j ];
				mixedPCM [ j * 2 + 1 ] = lrPCM [ 1 ] [ j ];
			}
			else mixedPCM [ j ] = lrPCM [ 0 ] [ j ];
		}

		*sample = new AEInternalLameSample ( mixedPCM, result * sizeof ( int16_t ) * channels,
			AETimeSpan::fromSeconds ( nextIndex - 1 * 0.026125 ), AETimeSpan::fromByteCount ( result, byterate ) );

		return AEERROR_SUCCESS;
	}

private:
	hip_t hip;

	int channels, samplerate, byterate;

	AEAutoPtr<AEBaseStream> stream;
	std::vector<POSITIONSIZE> posizeCache;
	int nextIndex;

	int64_t startPoint;
};

AEERROR AE_createLameMp3Decoder ( AEBaseAudioDecoder ** decoder )
{
	*decoder = new AEInternalLameDecoder ();
	return AEERROR_SUCCESS;
}