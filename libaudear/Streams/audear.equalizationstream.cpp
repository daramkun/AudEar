#include "../audear.h"

class AEInternalEqualizationStream : public AEBaseAudioStream
{
public:
	AEInternalEqualizationStream ( AEBaseAudioStream * baseStream, AEEqualizerBand * bands, uint32_t bandCount )
		: baseStream ( baseStream ), bandCount ( bandCount )
	{
		baseStream->getWaveFormat ( &waveFormat );

		this->bands = ( AEEqualizerBand * ) CoTaskMemAlloc ( sizeof ( AEEqualizerBand ) * bandCount );
		filters = ( AEBiQuadFilter * ) CoTaskMemAlloc ( sizeof ( AEBiQuadFilter ) * bandCount * waveFormat.getChannels () );

		updateFilters ( bands, bandCount );
	}
	AEInternalEqualizationStream ()
	{
		CoTaskMemFree ( filters );
		CoTaskMemFree ( bands );
	}

public:
	virtual AEERROR STDCALL read ( OUT void * buffer, int64_t length, OUT int64_t * readed )
	{
		AEERROR et = baseStream->read ( buffer, length, readed );
		if ( FAILED ( et ) ) return et;

		for ( int bandIndex = 0; bandIndex < bandCount; ++bandIndex )
			for ( int n = 0; n < waveFormat.getChannels (); ++n )
				filters [ bandIndex * waveFormat.getChannels () + n ].resetState ();

		if ( waveFormat.getWaveFormat () == AE_WAVEFORMAT_IEEE_FLOAT )
		{
			float * fBuffer = ( float * ) buffer;
			for ( int i = 0; i < *readed / sizeof ( float ); ++i )
			{
				int ch = i % waveFormat.getChannels ();
				for ( int b = 0; b < bandCount; ++b )
					fBuffer [ i ] = filters [ b * waveFormat.getChannels () + ch ].transform ( fBuffer [ i ] );
			}
		}
		else if ( waveFormat.getWaveFormat () == AE_WAVEFORMAT_PCM )
		{
			if ( waveFormat.getBitsPerSample () == 8 )
			{
				int8_t * bBuffer = ( int8_t * ) buffer;
				for ( int i = 0; i < *readed / sizeof ( int8_t ); ++i )
				{
					int ch = i % waveFormat.getChannels ();
					for ( int b = 0; b < bandCount; ++b )
					{
						float temp = filters [ b * waveFormat.getChannels () + ch ].transform ( bBuffer [ i ] / ( float ) CHAR_MAX ) * CHAR_MAX;
						if ( temp > CHAR_MAX ) temp = CHAR_MAX;
						if ( temp < CHAR_MIN ) temp = CHAR_MIN;
						bBuffer [ i ] = ( int8_t ) temp;
					}
				}
			}
			if ( waveFormat.getBitsPerSample () == 16 )
			{
				int16_t * bBuffer = ( int16_t * ) buffer;
				for ( int i = 0; i < *readed / sizeof ( int16_t ); ++i )
				{
					int ch = i % waveFormat.getChannels ();
					for ( int b = 0; b < bandCount; ++b )
					{
						float temp = filters [ b * waveFormat.getChannels () + ch ].transform ( bBuffer [ i ] / ( float ) SHRT_MAX ) * SHRT_MAX;
						if ( temp > SHRT_MAX ) temp = SHRT_MAX;
						if ( temp < SHRT_MIN ) temp = SHRT_MIN;
						bBuffer [ i ] = ( int16_t ) temp;
					}
				}
			}
			else if ( waveFormat.getBitsPerSample () == 32 )
			{
				int32_t * bBuffer = ( int32_t * ) buffer;
				for ( int i = 0; i < *readed / sizeof ( int32_t ); ++i )
				{
					int ch = i % waveFormat.getChannels ();
					for ( int b = 0; b < bandCount; ++b )
					{
						float temp = filters [ b * waveFormat.getChannels () + ch ].transform ( bBuffer [ i ] / ( float ) INT_MAX ) * INT_MAX;
						if ( temp > INT_MAX ) temp = INT_MAX;
						if ( temp < INT_MIN ) temp = INT_MIN;
						bBuffer [ i ] = ( int32_t ) temp;
					}
				}
			}
		}

		return AEERROR_SUCCESS;
	}
	virtual AEERROR STDCALL write ( IN const void * data, int64_t length, OUT int64_t * written )
	{
		return baseStream->write ( data, length, written );
	}
	virtual AEERROR STDCALL seek ( AESTREAMSEEK offset, int64_t count, OUT int64_t * seeked )
	{
		return baseStream->seek ( offset, count, seeked );
	}
	virtual AEERROR STDCALL flush ()
	{
		return baseStream->flush ();
	}

public:
	virtual AEERROR STDCALL getPosition ( OUT int64_t * pos )
	{
		return baseStream->getPosition ( pos );
	}
	virtual AEERROR STDCALL getLength ( OUT int64_t * len )
	{
		return baseStream->getLength ( len );
	}

public:
	virtual AEERROR STDCALL canSeek ( OUT bool * can ) { return baseStream->canSeek ( can ); }
	virtual AEERROR STDCALL canRead ( OUT bool * can ) { return baseStream->canRead ( can ); }
	virtual AEERROR STDCALL canWrite ( OUT bool * can ) { return baseStream->canWrite ( can ); }

public:
	virtual AEERROR STDCALL getBaseDecoder ( OUT AEBaseAudioDecoder ** decoder )
	{
		return baseStream->getBaseDecoder ( decoder );
	}
	virtual AEERROR STDCALL getWaveFormat ( OUT AEWaveFormat * format )
	{
		return baseStream->getWaveFormat ( format );
	}

public:
	virtual AEERROR STDCALL setBufferSize ( AETimeSpan length )
	{
		return baseStream->setBufferSize ( length );
	}
	virtual AEERROR STDCALL getBufferSize ( OUT AETimeSpan * length )
	{
		return baseStream->getBufferSize ( length );
	}

public:
	virtual AEERROR STDCALL buffering ()
	{
		return baseStream->buffering ();
	}

public:
	AEERROR updateFilters ( AEEqualizerBand * bands, uint32_t bandCount )
	{
		if ( this->bandCount != bandCount )
			return AEERROR_INVALID_ARGUMENT;

		memcpy ( this->bands, bands, bandCount * sizeof ( AEEqualizerBand ) );

		for ( int bandIndex = 0; bandIndex < bandCount; ++bandIndex )
		{
			for ( int n = 0; n < waveFormat.getChannels (); ++n )
			{
				filters [ bandIndex * waveFormat.getChannels () + n ].resetState ();
				filters [ bandIndex * waveFormat.getChannels () + n ].setPeakingEq ( waveFormat.getSampleRate (),
					bands [ bandIndex ].frequency, bands [ bandIndex ].bandwidth, bands [ bandIndex ].gain );
			}
		}

		return AEERROR_SUCCESS;
	}

private:
	AEAutoPtr<AEBaseAudioStream> baseStream;
	AEWaveFormat waveFormat;

	AEEqualizerBand * bands;
	uint32_t bandCount;

	AEBiQuadFilter * filters;
};

AEERROR AE_createEqualizationAudioStream ( AEBaseAudioStream * baseStream, AEEqualizerBand * bands, uint32_t bandCount, AEBaseAudioStream ** stream )
{
	*stream = new AEInternalEqualizationStream ( baseStream, bands, bandCount );
	return AEERROR_SUCCESS;
}

AEERROR AE_updateEqualizationAudioStream ( AEBaseAudioStream * stream, AEEqualizerBand * bands, uint32_t bandCount )
{
	return static_cast< AEInternalEqualizationStream* >( stream )->updateFilters ( bands, bandCount );
}