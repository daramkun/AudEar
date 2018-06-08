#ifndef __AUDEAR_STRUCT_H__
#define __AUDEAR_STRUCT_H__

#include <string>

typedef int													wavefmt_t;
#define AE_WAVEFORMAT_UNKNOWN								0
#define AE_WAVEFORMAT_PCM									1
#define AE_WAVEFORMAT_IEEE_FLOAT							2

// Wave Format
struct AEEXP AEWaveFormat
{
public:
	inline AEWaveFormat () : channels ( 0 ), bitsPerSample ( 0 ), samplePerSec ( 0 ), format ( AE_WAVEFORMAT_UNKNOWN ) { }
	inline AEWaveFormat ( int channels, int bitsPerSample, int samplePerSec, wavefmt_t format = AE_WAVEFORMAT_PCM )
		: channels ( channels ), bitsPerSample ( bitsPerSample ), samplePerSec ( samplePerSec ), format ( format )
	{ }
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
	AEWaveFormat ( WAVEFORMATEX * pwfx );
#endif

public:
	inline int getChannels () const { return channels; }
	inline int getBitsPerSample () const { return bitsPerSample; }
	inline int getSampleRate () const { return samplePerSec; }
	inline int getBlockAlignment () const { return channels * ( bitsPerSample / 8 ); }
	inline int getByteRate () const { return getBlockAlignment () * samplePerSec; }
	inline wavefmt_t getWaveFormat () const { return format; }

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
	WAVEFORMATEX * getWaveFormatEx () const;
#endif

public:
	inline bool operator== ( const AEWaveFormat & format )
	{
		return ( channels == format.channels && bitsPerSample == format.bitsPerSample
			&& samplePerSec == format.samplePerSec && this->format == format.format );
	}

private:
	int channels;
	int bitsPerSample;
	int samplePerSec;
	wavefmt_t format;
};

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

// Nano-seconds Time Span structure
// Compatible to .NET TimeSpan struct object
struct AEEXP AETimeSpan
{
public:
	inline static AETimeSpan maxValue () { return AETimeSpan ( 0x7fffffffffffffffLL ); }
	inline static AETimeSpan minValue () { return AETimeSpan ( 0x8000000000000000LL ); }

public:
	inline AETimeSpan ( int64_t ticks = 0 ) : _ticks ( ticks ) { }
	inline AETimeSpan ( int hour, int minute, int second, int millisec )
	{
		int millisecs = ( hour * 3600 + minute * 60 + second ) * MILLISECS_PER_SECOND + millisec;
		_ticks = millisecs * TICKS_PER_MILLISECOND;
	}

public:
	inline int64_t getTicks () const { return _ticks; }
	inline int getMilliseconds () const { return ( _ticks / TICKS_PER_MILLISECOND ) % 1000; }
	inline int getSeconds () const { return ( _ticks / TICKS_PER_SECOND ) % 60; }
	inline int getMinutes () const { return ( _ticks / TICKS_PER_MINUTE ) % 60; }
	inline int getHours () const { return ( _ticks / TICKS_PER_HOUR ) % 24; }
	inline int getDays () const { return ( int ) ( _ticks / TICKS_PER_DAY ); }

	inline double totalMilliseconds () const { return _ticks * MILLISECS_PER_TICK; }
	inline double totalSeconds () const { return _ticks * SECONDS_PER_TICK; }
	inline double totalMinutes () const { return _ticks * MINUTES_PER_TICK; }
	inline double totalHours () const { return _ticks * HOURS_PER_TICK; }
	inline double totalDays () const { return _ticks * DAYS_PER_TICK; }

	inline int64_t getByteCount ( int byterate ) const
	{
		return ( int64_t ) ( totalSeconds () * byterate );
	}
	inline int64_t getByteCount ( const AEWaveFormat & waveFormat ) const
	{
		return getByteCount ( waveFormat.getByteRate () );
	}

public:
	inline operator const int64_t () const { return _ticks; }
	inline operator std::string () const
	{
		char buffer [ 4096 ];
		sprintf_s ( buffer, "%02d:%02d:%02d.%03d", getHours (), getMinutes (), getSeconds (), getMilliseconds () );
		return buffer;
	}
	inline operator std::wstring () const
	{
		wchar_t buffer [ 4096 ];
		wsprintf ( buffer, L"%02d:%02d:%02d.%03d", getHours (), getMinutes (), getSeconds (), getMilliseconds () );
		return buffer;
	}
	inline AETimeSpan operator+ ( const AETimeSpan & a2 ) const { return AETimeSpan ( _ticks + a2._ticks ); }
	inline AETimeSpan& operator+= ( const AETimeSpan & a2 ) { _ticks += a2._ticks; return *this; }
	inline AETimeSpan operator- ( const AETimeSpan & a2 ) const { return AETimeSpan ( _ticks - a2._ticks ); }
	inline AETimeSpan& operator-= ( const AETimeSpan & a2 ) { _ticks -= a2._ticks; return *this; }
	inline bool operator == ( const AETimeSpan & a2 ) const { return _ticks == a2._ticks; }
	inline bool operator < ( const AETimeSpan & a2 ) const { return _ticks < a2._ticks; }
	inline bool operator > ( const AETimeSpan & a2 ) const { return _ticks > a2._ticks; }
	inline bool operator <= ( const AETimeSpan & a2 ) const { return _ticks <= a2._ticks; }
	inline bool operator >= ( const AETimeSpan & a2 ) const { return _ticks >= a2._ticks; }

public:
	static inline AETimeSpan fromMillseconds ( double millisecs ) { return AETimeSpan ( ( int64_t ) ( millisecs * TICKS_PER_MILLISECOND ) ); }
	static inline AETimeSpan fromSeconds ( double seconds ) { return AETimeSpan ( ( int64_t ) ( seconds * MILLISECS_PER_SECOND * TICKS_PER_MILLISECOND ) ); }
	static inline AETimeSpan fromMinutes ( double minutes ) { return AETimeSpan ( ( int64_t ) ( minutes * 60 * MILLISECS_PER_SECOND * TICKS_PER_MILLISECOND ) ); }
	static inline AETimeSpan fromHours ( double hours ) { return AETimeSpan ( ( int64_t ) ( hours * 60 * 60 * MILLISECS_PER_SECOND * TICKS_PER_MILLISECOND ) ); }

	static inline AETimeSpan fromByteCount ( int64_t byteCount, int byterate )
	{
		return fromSeconds ( byteCount / ( double ) byterate );
	}
	static inline AETimeSpan fromByteCount ( int64_t byteCount, const AEWaveFormat & format )
	{
		return fromByteCount ( byteCount, format.getByteRate () );
	}

private:
	int64_t _ticks;
};

#if !( AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP )
#include <atomic>
#endif

struct AESpinLock
{
public:
	inline AESpinLock ()
	{
#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
		InitializeSRWLock ( &spinLock );
#	else
		atomic_flag = ATOMIC_FLAG_INIT;
#	endif
	}

public:
	inline void lock ()
	{
#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
		AcquireSRWLockExclusive ( &spinLock );
#	else
		while ( spinLock.test_and_set ( std::memory_order_acquire ) )
			;
#	endif
	}
	inline void unlock ()
	{
#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
		ReleaseSRWLockExclusive ( &spinLock );
#	else
		spinLock.clear ( std::memory_order_release );
#	endif
	}

private:
#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
	SRWLOCK spinLock;
#	else
	std::atomic_flag spinLock;
#	endif
};

#endif