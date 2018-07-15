#ifndef __AUDEAR_HIGHRESOLUTIONTIMER_HPP__
#define __AUDEAR_HIGHRESOLUTIONTIMER_HPP__

#include <cstdint>

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
#	include <Windows.h>
#	pragma comment ( lib, "winmm.lib" )
#else
#	include <sys/time.h>
#endif

static inline int64_t __HRT_GetFrequency ()
{
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
	LARGE_INTEGER freq;
	if ( !QueryPerformanceFrequency ( &freq ) )
		return 1000 * 10000;
	return freq.QuadPart;
#else
	return 10000 * 1000;
#endif
}

static inline int64_t __HRT_GetCounter ()
{
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
	LARGE_INTEGER counter;
	if ( !QueryPerformanceCounter ( &counter ) )
		return timeGetTime () * 10000;
	return counter.QuadPart;
#else
	timeval time;
	gettimeofday ( &time, nullptr );
	return ( int64_t ) ( time.tv_sec * 1000 * 10000 );
#endif
}

#endif