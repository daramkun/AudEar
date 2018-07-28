#define _USE_MATH_DEFINES
#include "../audear.h"

#include <complex>
#include <cmath>

void __swap ( std::complex<double> & a, std::complex<double> & b )
{
	std::complex<double> temp = a;
	a = b;
	b = temp;
}

void __fft ( std::complex<double> * complexes, int count, bool isInv )
{
	int n = count;
	for ( int i = 1, j = 0; i < n; ++i )
	{
		int bit = n >> 1;
		while ( !( ( j ^= bit ) & bit ) )
			bit >>= 1;
		if ( i < j )
			__swap ( complexes [ i ], complexes [ j ] );
	}

	for ( int i = 1; i < n; i <<= 1 )
	{
		double x = isInv ? M_PI / i : -M_PI / i;
		std::complex<double> w = ( cos ( x ), sin ( x ) );
		for ( int j = 0; j < n; j += i << 1 )
		{
			std::complex<double> th ( 1, 0 );
			for ( int k = 0; k < i; ++k )
			{
				std::complex<double> tmp = complexes [ i + j + k ] * th;
				complexes [ i + j + k ] = complexes [ j + k ] - tmp;
				complexes [ j + k ] += tmp;
				th *= w;
			}
		}
	}

	if ( isInv )
		for ( int i = 0; i < n; ++i )
			complexes [ i ] /= n;
}

int __chConv [] = { AESC_CH1, AESC_CH2, AESC_CH3, AESC_CH4, AESC_CH5, AESC_CH6, AESC_CH7, AESC_CH8, AESC_CH9, AESC_CH10, };

class __FrequencySpectrumAudioStream
{
public:
	__FrequencySpectrumAudioStream ( AEAUDIOSTREAM * stream, AEWAVEFORMAT wf, void ( *callback )( AESPECTRUMCHANNELS ch, float *freqs, int freqsCount ), AESPECTRUMCHANNELS ch )
		: _stream ( stream ), _wf ( wf ), _callback ( callback ), _ch ( ch )
	{
		AE_retainInterface ( stream );

	}
	~__FrequencySpectrumAudioStream ()
	{
		AE_releaseInterface ( ( void ** ) &_stream );
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		return _stream->getWaveFormat ( _stream->object, format );
	}
	error_t buffering () noexcept
	{
		return _stream->buffering ( _stream->object );
	}

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		int64_t ret = _stream->read ( _stream->object, buffer, len );
		if ( ret <= 0 ) return 0;
		if ( ret % _wf.blockAlign != 0 ) return -1;

		if ( _ch != AESC_NONE && _callback != nullptr )
		{
			float * floatBuffer = ( float * ) buffer;
			std::shared_ptr<std::complex<double> []> complexBuffer ( new std::complex<double> [ ret / 4 ] );
			for ( int i = 0; i < ret / 4; i += _wf.channels )
			{
				float value = 0;
				int avg = 0;
				for ( int ch = 0; ch < _wf.channels; ++ch )
				{
					if ( _ch & __chConv [ ch ] || _ch == AESC_AVGALL )
					{
						value += floatBuffer [ i + ch ];
						++avg;
					}
				}
				complexBuffer [ i / _wf.channels ] = std::complex<double> ( value / avg, 0 );
			}
			__fft ( &complexBuffer [ 0 ], ( int ) ret / 4, false );

			std::shared_ptr<float [] > complexValueBuffer ( new float [ ret / 4 ] );
			for ( int i = 0; i < ret / 4; ++i )
				complexValueBuffer [ i ] = ( float ) sqrt ( pow ( complexBuffer [ i ].real (), 2 ) + pow ( complexBuffer [ i ].imag (), 2 ) );

			_callback ( _ch, &complexValueBuffer [ 0 ], ( int ) ret / 4 );
		}

		return ret;
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, offset, origin );
	}
	int64_t tell () noexcept
	{
		return _stream->tell ( _stream->object );
	}
	int64_t length () noexcept
	{
		return _stream->length ( _stream->object );
	}

private:
	AEAUDIOSTREAM * _stream;
	AEWAVEFORMAT _wf;
	void ( *_callback )( AESPECTRUMCHANNELS ch, float *freqs, int freqsCount );
	AESPECTRUMCHANNELS _ch;
};

error_t AE_createFrequencySpectrumAudioStream ( AEAUDIOSTREAM * stream, void ( *callback )( AESPECTRUMCHANNELS ch, float *freqs, int freqsCount ), AESPECTRUMCHANNELS ch, AEAUDIOSTREAM ** ret )
{
	if ( stream == nullptr || ret == nullptr ) return AEERROR_ARGUMENT_IS_NULL;

	AEWAVEFORMAT wf;
	stream->getWaveFormat ( stream->object, &wf );
	if ( wf.audioFormat != AEAF_IEEE_FLOAT )
		return AEERROR_NOT_SUPPORTED_FORMAT;

	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );

	audioStream->object = new __FrequencySpectrumAudioStream ( stream, wf, callback, ch );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __FrequencySpectrumAudioStream* >( obj ); };
	audioStream->tag = "AudEar Frequency Spectrum Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __FrequencySpectrumAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __FrequencySpectrumAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __FrequencySpectrumAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __FrequencySpectrumAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __FrequencySpectrumAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __FrequencySpectrumAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}

class __DecibelSpectrumAudioStream
{
public:
	__DecibelSpectrumAudioStream ( AEAUDIOSTREAM * stream, AEWAVEFORMAT wf, void ( *callback )( AESPECTRUMCHANNELS ch, float *dbs, int dbsCount ), AESPECTRUMCHANNELS ch )
		: _stream ( stream ), _wf ( wf ), _callback ( callback ), _ch ( ch )
	{
		AE_retainInterface ( stream );

	}
	~__DecibelSpectrumAudioStream ()
	{
		AE_releaseInterface ( ( void ** ) &_stream );
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		return _stream->getWaveFormat ( _stream->object, format );
	}
	error_t buffering () noexcept
	{
		return _stream->buffering ( _stream->object );
	}

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		int64_t ret = _stream->read ( _stream->object, buffer, len );
		if ( ret <= 0 ) return 0;
		if ( ret % _wf.blockAlign != 0 ) return -1;

		if ( _ch != AESC_NONE && _callback != nullptr )
		{
			float * floatBuffer = ( float * ) buffer;
			std::shared_ptr<float []> dbBuffer ( new float [ ret / 4 ] );
			for ( int i = 0; i < ret / 4; i += _wf.channels )
			{
				float value = 0;
				int avg = 0;
				for ( int ch = 0; ch < _wf.channels; ++ch )
				{
					if ( _ch & __chConv [ ch ] || _ch == AESC_AVGALL )
					{
						value += floatBuffer [ i + ch ];
						++avg;
					}
				}
				dbBuffer [ i / _wf.channels ] = 20 * log10 ( abs ( value / avg ) );
			}
			_callback ( _ch, &dbBuffer [ 0 ], ( int ) ret / 4 );
		}

		return ret;
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, offset, origin );
	}
	int64_t tell () noexcept
	{
		return _stream->tell ( _stream->object );
	}
	int64_t length () noexcept
	{
		return _stream->length ( _stream->object );
	}

private:
	AEAUDIOSTREAM * _stream;
	AEWAVEFORMAT _wf;
	void ( *_callback )( AESPECTRUMCHANNELS ch, float *freqs, int freqsCount );
	AESPECTRUMCHANNELS _ch;
};

error_t AE_createDecibelSpectrumAudioStream ( AEAUDIOSTREAM * stream, void ( *callback ) ( AESPECTRUMCHANNELS ch, float * dbs, int dbsCount ), AESPECTRUMCHANNELS ch, AEAUDIOSTREAM ** ret )
{
	if ( stream == nullptr || ret == nullptr ) return AEERROR_ARGUMENT_IS_NULL;

	AEWAVEFORMAT wf;
	stream->getWaveFormat ( stream->object, &wf );
	if ( wf.audioFormat != AEAF_IEEE_FLOAT )
		return AEERROR_NOT_SUPPORTED_FORMAT;

	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );

	audioStream->object = new __DecibelSpectrumAudioStream ( stream, wf, callback, ch );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __DecibelSpectrumAudioStream* >( obj ); };
	audioStream->tag = "AudEar Decibel Spectrum Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __DecibelSpectrumAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __DecibelSpectrumAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __DecibelSpectrumAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __DecibelSpectrumAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __DecibelSpectrumAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __DecibelSpectrumAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}