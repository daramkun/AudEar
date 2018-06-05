#ifndef __AUDEAR_STRUCT_H__
#define __AUDEAR_STRUCT_H__

#include <string>

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
	inline static AETimeSpan MaxValue () { return AETimeSpan ( 0x7fffffffffffffffLL ); }
	inline static AETimeSpan MinValue () { return AETimeSpan ( 0x8000000000000000LL ); }

public:
	inline AETimeSpan ( LONGLONG ticks = 0 ) : _ticks ( ticks ) { }
	inline AETimeSpan ( int hour, int minute, int second, int millisec )
	{
		int millisecs = ( hour * 3600 + minute * 60 + second ) * MILLISECS_PER_SECOND + millisec;
		_ticks = millisecs * TICKS_PER_MILLISECOND;
	}

public:
	inline LONGLONG GetTicks () const { return _ticks; }
	inline int GetMilliseconds () const { return ( _ticks / TICKS_PER_MILLISECOND ) % 1000; }
	inline int GetSeconds () const { return ( _ticks / TICKS_PER_SECOND ) % 60; }
	inline int GetMinutes () const { return ( _ticks / TICKS_PER_MINUTE ) % 60; }
	inline int GetHours () const { return ( _ticks / TICKS_PER_HOUR ) % 24; }
	inline int GetDays () const { return ( int ) ( _ticks / TICKS_PER_DAY ); }

	inline double TotalMilliseconds () const { return _ticks * MILLISECS_PER_TICK; }
	inline double TotalSeconds () const { return _ticks * SECONDS_PER_TICK; }
	inline double TotalMinutes () const { return _ticks * MINUTES_PER_TICK; }
	inline double TotalHours () const { return _ticks * HOURS_PER_TICK; }
	inline double TotalDays () const { return _ticks * DAYS_PER_TICK; }

	inline LONGLONG GetBytes ( int byterate ) const
	{
		return ( LONGLONG ) ( TotalSeconds () * byterate );
	}

public:
	inline operator const LONGLONG () const { return _ticks; }
	inline operator std::string () const
	{
		char buffer [ 4096 ];
		sprintf_s ( buffer, "%02d:%02d:%02d.%03d", GetHours (), GetMinutes (), GetSeconds (), GetMilliseconds () );
		return buffer;
	}
	inline operator std::wstring () const
	{
		wchar_t buffer [ 4096 ];
		wsprintf ( buffer, L"%02d:%02d:%02d.%03d", GetHours (), GetMinutes (), GetSeconds (), GetMilliseconds () );
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
	static inline AETimeSpan FromMillseconds ( double millisecs ) { return AETimeSpan ( ( LONGLONG ) ( millisecs * TICKS_PER_MILLISECOND ) ); }
	static inline AETimeSpan FromSeconds ( double seconds ) { return AETimeSpan ( ( LONGLONG ) ( seconds * MILLISECS_PER_SECOND * TICKS_PER_MILLISECOND ) ); }
	static inline AETimeSpan FromMinutes ( double minutes ) { return AETimeSpan ( ( LONGLONG ) ( minutes * 60 * MILLISECS_PER_SECOND * TICKS_PER_MILLISECOND ) ); }
	static inline AETimeSpan FromHours ( double hours ) { return AETimeSpan ( ( LONGLONG ) ( hours * 60 * 60 * MILLISECS_PER_SECOND * TICKS_PER_MILLISECOND ) ); }

	static inline AETimeSpan FromByteCount ( LONGLONG bytes, int byterate )
	{
		return FromSeconds ( bytes / ( double ) byterate );
	}

private:
	LONGLONG _ticks;
};

#endif