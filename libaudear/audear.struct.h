#ifndef __AUDEAR_STRUCT_H__
#define __AUDEAR_STRUCT_H__

#include <stdint.h>
#include <malloc.h>

typedef enum
{
	AESC_NONE = 0,
	AESC_CH1 = 1 << 0,
	AESC_CH2 = 1 << 1,
	AESC_CH3 = 1 << 2,
	AESC_CH4 = 1 << 3,
	AESC_CH5 = 1 << 4,
	AESC_CH6 = 1 << 5,
	AESC_CH7 = 1 << 6,
	AESC_CH8 = 1 << 7,
	AESC_CH9 = 1 << 8,
	AESC_CH10 = 1 << 9,
	AESC_AVGALL = 0xffffffff,
} AESPECTRUMCHANNELS;

typedef enum
{
	AEAF_UNKNOWN,
	AEAF_PCM,
	AEAF_IEEE_FLOAT,
} AEAUDIOFORMAT;

typedef struct
{
	uint16_t channels, bitsPerSample, blockAlign;
	uint32_t samplesPerSec, bytesPerSec;
	AEAUDIOFORMAT audioFormat;
} AEWAVEFORMAT;

INLINE AEWAVEFORMAT AEWAVEFORMAT_initialize ( int channels, int bitsPerSample, int samplesPerSec, AEAUDIOFORMAT audioFormat )
{
	AEWAVEFORMAT wf;
	wf.channels = channels;
	wf.bitsPerSample = bitsPerSample;
	wf.blockAlign = channels * ( bitsPerSample / 8 );
	wf.samplesPerSec = samplesPerSec;
	wf.bytesPerSec = wf.blockAlign * samplesPerSec;
	wf.audioFormat = audioFormat;
	return wf;
}
#if AE_PLATFORM_WINDOWS
EXTC AEEXP AEWAVEFORMAT AEWAVEFORMAT_waveFormatFromWaveFormatEX ( const WAVEFORMATEX * pwfx );
EXTC AEEXP WAVEFORMATEX * AEWAVEFORMAT_waveFormatExFromWaveFormat ( const AEWAVEFORMAT * waveFormat );
#endif

INLINE uint16_t AEWAVEFORMAT_calculateBlockAlignment ( uint16_t channels, uint16_t bitsPerSample )
{
	return channels * ( bitsPerSample / 8 );
}
INLINE uint32_t AEWAVEFORMAT_calculateByteRate ( uint16_t blockAlign, uint32_t sampleRate )
{
	return blockAlign * sampleRate;
}

typedef struct
{
	int64_t ticks;
} AETIMESPAN;

#define TICKS_PER_MILLISECOND								10000LL
#define TICKS_PER_SECOND									( TICKS_PER_MILLISECOND * 1000LL )
#define TICKS_PER_MINUTE									( TICKS_PER_SECOND * 60LL )
#define TICKS_PER_HOUR										( TICKS_PER_MINUTE * 60LL )
#define TICKS_PER_DAY										( TICKS_PER_HOUR * 24LL )

#define MILLISECS_PER_TICK									( 1.0 / TICKS_PER_MILLISECOND )
#define SECONDS_PER_TICK									( 1.0 / TICKS_PER_SECOND )
#define MINUTES_PER_TICK									( 1.0 / TICKS_PER_MINUTE )
#define HOURS_PER_TICK										( 1.0 / TICKS_PER_HOUR )
#define DAYS_PER_TICK										( 1.0 / TICKS_PER_DAY )

#define MILLISECS_PER_SECOND								1000LL
#define MILLISECS_PER_MINUTE								( MILLISEC_PER_SECOND * 60LL )
#define MILLISECS_PER_HOUR									( MILLISEC_PER_MINUTE * 60LL )
#define MILLISECS_PER_DAY									( MILLISEC_PER_HOUR * 24LL )

INLINE AETIMESPAN AETIMESPAN_initializeWithTicks ( int64_t ticks )
{
	AETIMESPAN ret = { ticks };
	return ret;
}
INLINE AETIMESPAN AETIMESPAN_initializeWithTimes ( int16_t hour, int16_t minute, int16_t second, int16_t millisec )
{
	int millisecs = ( hour * 3600 + minute * 60 + second ) * MILLISECS_PER_SECOND + millisec;
	AETIMESPAN ret = { millisecs * TICKS_PER_MILLISECOND };
	return ret;
}
INLINE AETIMESPAN AETIMESPAN_initializeWithSeconds ( double seconds )
{
	AETIMESPAN ret = { ( int64_t ) ( seconds * MILLISECS_PER_SECOND * TICKS_PER_MILLISECOND ) };
	return ret;
}
INLINE AETIMESPAN AETIMESPAN_initializeWithByteCount ( int64_t bytes, uint32_t byterate )
{
	return AETIMESPAN_initializeWithSeconds ( bytes / ( double ) byterate );
}
INLINE AETIMESPAN AETIMESPAN_initializeWithByteCountAndWaveFormat ( int64_t bytes, const AEWAVEFORMAT * waveFormat )
{
	return AETIMESPAN_initializeWithSeconds ( bytes / ( double ) waveFormat->bytesPerSec );
}
INLINE int64_t AETIMESPAN_getTicks ( AETIMESPAN timeSpan )
{
	return timeSpan.ticks;
}
INLINE int32_t AETIMESPAN_getMillisecs ( AETIMESPAN timeSpan )
{
	return ( timeSpan.ticks / TICKS_PER_MILLISECOND ) % 1000;
}
INLINE int32_t AETIMESPAN_getSeconds ( AETIMESPAN timeSpan )
{
	return ( timeSpan.ticks / TICKS_PER_SECOND ) % 60;
}
INLINE int32_t AETIMESPAN_getMinutes ( AETIMESPAN timeSpan )
{
	return ( timeSpan.ticks / TICKS_PER_MINUTE ) % 60;
}
INLINE int32_t AETIMESPAN_getHours ( AETIMESPAN timeSpan )
{
	return ( timeSpan.ticks / TICKS_PER_HOUR ) % 24;
}
INLINE double AETIMESPAN_totalMillisecs ( AETIMESPAN timeSpan )
{
	return timeSpan.ticks * MILLISECS_PER_TICK;
}
INLINE double AETIMESPAN_totalSeconds ( AETIMESPAN timeSpan )
{
	return timeSpan.ticks * SECONDS_PER_TICK;
}
INLINE double AETIMESPAN_totalMinutes ( AETIMESPAN timeSpan )
{
	return timeSpan.ticks * MINUTES_PER_TICK;
}
INLINE double AETIMESPAN_totalHours ( AETIMESPAN timeSpan )
{
	return timeSpan.ticks * HOURS_PER_TICK;
}
INLINE int64_t AETIMESPAN_getBytesCount ( AETIMESPAN timeSpan, int byterate )
{
	return ( int64_t ) ( AETIMESPAN_totalSeconds ( timeSpan ) * byterate );
}

typedef struct AEEXP _AEBIQUADFILTER
{
	double a0, a1, a2, a3, a4;
	double x1 [ 12 ], x2 [ 12 ], y1 [ 12 ], y2 [ 12 ];
} AEBIQUADFILTER;

EXTC AEEXP float AEBIQUADFILTER_process ( AEBIQUADFILTER * f, float input, int ch );

EXTC AEEXP AEBIQUADFILTER AE_initializeHighPassFilter ( int samplerate, double freq, double q );
EXTC AEEXP AEBIQUADFILTER AE_initializeHighShelfFilter ( int samplerate, double freq, float shelfSlope, double gaindb );
EXTC AEEXP AEBIQUADFILTER AE_initializeLowPassFilter ( int samplerate, double freq, double q );
EXTC AEEXP AEBIQUADFILTER AE_initializeLowShelfFilter ( int samplerate, double freq, float shelfSlope, double gaindb );
EXTC AEEXP AEBIQUADFILTER AE_initializeAllPassFilter ( int samplerate, double freq, float q );
EXTC AEEXP AEBIQUADFILTER AE_initializeNotchFilter ( int samplerate, double freq, double q );
EXTC AEEXP AEBIQUADFILTER AE_initializePeakEqFilter ( int samplerate, double freq, double bandwidth, double peakGainDB );
EXTC AEEXP AEBIQUADFILTER AE_initializeBandPassFilterCSG ( int samplerate, double freq, double q );
EXTC AEEXP AEBIQUADFILTER AE_initializeBandPassFilterCPG ( int samplerate, double freq, double q );

typedef struct AEEXP
{
	AEBIQUADFILTER filters [ 16 ];
	int8_t filters_size;
} AEFILTERCOLLECTION;

typedef enum AEEXP
{
	AEEQP_NONE,
	AEEQP_ACOUSTIC,
	AEEQP_DANCE,
	AEEQP_JAZZ,
	AEEQP_ELECTRONIC,
	AEEQP_POP,
	AEEQP_ROCK,
	AEEQP_HIPHOP,
	AEEQP_CLASSICAL,
	AEEQP_PIANO,
} AEEQUALIZERPRESET;

EXTC AEEXP AEFILTERCOLLECTION AE_initializeSingleItemFilterCollection ( AEBIQUADFILTER filter );
EXTC AEEXP AEFILTERCOLLECTION AE_initializeEqualizerFilterCollection ( int samplerate, double bandwidth, AEEQUALIZERPRESET preset );
EXTC AEEXP AEFILTERCOLLECTION AE_initializeEqualizerFilterCollectionWithGainDB ( int samplerate, double bandwidth, double * gainDBs );

#ifdef __cplusplus
extern "C++"
{
	template<typename T>
	struct AEAUDIOBUFFER
	{
	private:
		T * buffer;
		int64_t size;

	public:
		inline AEAUDIOBUFFER () { buffer = nullptr; }
		inline AEAUDIOBUFFER ( int64_t size )
			: size ( size )
		{
			buffer = ( T* ) _mm_malloc ( ( size_t ) size * sizeof ( T ), 16 );
		}
		inline ~AEAUDIOBUFFER ()
		{
			if ( buffer )
				_mm_free ( buffer );
			buffer = nullptr;
		}

	public:
		inline operator T* ( ) noexcept { return buffer; }
		template<typename T2>
		inline operator T2* ( ) noexcept { return ( T2* ) buffer; }

	public:
		inline void resize ( int64_t size )
		{
			if ( buffer ) _mm_free ( buffer );
			buffer = ( T* ) _mm_malloc ( ( size_t ) size * sizeof ( T ), 16 );
			this->size = size;
		}
		inline void resizeAndCopy ( int64_t size )
		{
			T * temp = buffer;
			buffer = ( T* ) _mm_malloc ( ( size_t ) size * sizeof ( T ), 16 );
			if ( temp )
			{
				memcpy ( buffer, temp, ( size_t ) min ( this->size, size ) );
				_mm_free ( temp );
			}
			this->size = size;
		}

	public:
		inline void swap ( AEAUDIOBUFFER<T> & buffer )
		{
			T * bufferBuffer = buffer.buffer;
			int64_t bufferSize = buffer.size;

			buffer.buffer = this->buffer;
			buffer.size = size;

			this->buffer = bufferBuffer;
			size = bufferSize;
		}
	};
}
#endif

#endif