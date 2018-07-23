#ifndef __AUDEAR_TYPE_CONVERTER_HPP__
#define __AUDEAR_TYPE_CONVERTER_HPP__

/***************************************************************************************************
*
* TypeConverter.hpp
*   - 2018-07-20 Jin Jae-yeon
*   - Functions module for PCM audio data converter
*
***************************************************************************************************/

#include <cstdint>
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
//  SSE 2
#	include <mmintrin.h>
#	include <emmintrin.h>
//  SSE 3
#	include <immintrin.h>
#	include <pmmintrin.h>
//  SSE 4
#	include <smmintrin.h>
#endif

#pragma pack ( push, 1 )
struct int24_t
{
	union
	{
		int8_t value [ 3 ];
		struct { int16_t e1; int8_t e2; };
	};
};
#pragma pack ( pop )
#define INT24TOINT32(x)										( ( ( ( int32_t ) x.value [ 1 ] ) << 24 ) & 0xff0000 ) + ( ( ( ( int32_t ) x.value [ 2 ] ) << 16 ) & 0xff00 ) + x.value [ 0 ]

#define __TC_CHAR											SCHAR_MAX.0f
#define __TC_SHORT											SHRT_MAX.0f
#define __TC_24BIT											8388607.0f
#define __TC_INT											INT_MAX.0f

#define __TC_INV_CHAR										0.0078125
#define __TC_INV_SHORT										3.0518509475997192297128208258309e-5
#define __TC_INV_24BIT										1.1920930376163765926810017443897e-7
#define __TC_INV_INT										4.656612875245796924105750827168e-10

#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
static __m128i __g_TC_int8_to_int16_converter = _mm_set1_epi16 ( ( short ) ( __TC_SHORT / __TC_CHAR ) );
static __m128i __g_TC_int8_to_int24_converter = _mm_set1_epi32 ( ( int ) ( __TC_24BIT / __TC_CHAR ) );
static __m128i __g_TC_int8_to_int32_converter = _mm_set1_epi32 ( ( int ) ( __TC_INT / __TC_CHAR ) );
static __m128  __g_TC_int8_to_float_converter = _mm_set1_ps ( ( float ) __TC_INV_CHAR );

static __m128i __g_TC_int16_to_int24_converter = _mm_set1_epi32 ( ( int ) ( __TC_24BIT / __TC_SHORT ) );
static __m128i __g_TC_int16_to_int32_converter = _mm_set1_epi32 ( ( int ) ( __TC_INT / __TC_SHORT ) );
static __m128  __g_TC_int16_to_float_converter = _mm_set1_ps ( ( float ) __TC_INV_SHORT );

static __m128i __g_TC_int24_to_int32_converter = _mm_set1_epi32 ( ( int ) ( __TC_INT / __TC_24BIT ) );
static __m128  __g_TC_int24_to_float_converter = _mm_set1_ps ( ( float ) __TC_INV_24BIT );

static __m128  __g_TC_int32_to_float_converter = _mm_set1_ps ( ( float ) __TC_INV_INT );

static __m128 __g_TC_float_to_int8_converter = _mm_set1_ps ( ( float ) __TC_CHAR );
static __m128 __g_TC_float_to_int16_converter = _mm_set1_ps ( ( float ) __TC_SHORT );
static __m128 __g_TC_float_to_int24_converter = _mm_set1_ps ( ( float ) __TC_24BIT );
static __m128 __g_TC_float_to_int32_converter = _mm_set1_ps ( ( float ) __TC_INV_INT );

static __m128i __g_TC_zero_int = _mm_setzero_si128 ();
static __m128  __g_TC_zero_float = _mm_setzero_ps ();

static __m128 __g_TC_min_value = _mm_set1_ps ( 1 );
static __m128 __g_TC_max_value = _mm_set1_ps ( -1 );

static __m128i __g_TC_shuffle_int32_to_int8 = _mm_setr_epi8 ( 0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 );
#endif

typedef bool ( *__TypeConverterFunction ) ( void * src, void * dest, int srcByteCount, int destByteSize );

////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Converters of 8-bit PCM data to
// 
////////////////////////////////////////////////////////////////////////////////////////////////////

// 8-bit PCM data to 16-bit PCM data
extern "C" static inline bool __TC_int8_to_int16 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	static __m128i converter = _mm_set1_epi16 ( 256 );
#endif

	if ( srcByteCount * 2 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int loopCount = srcByteCount / 8 * 8;
	for ( int i = 0; i < loopCount; i += 8 )
	{
		__m128i loaded = _mm_cvtepi8_epi16 ( _mm_loadl_epi64 ( ( __m128i* ) ( srcBuffer + i ) ) );
		loaded = _mm_mullo_epi16 ( loaded, converter );
		_mm_store_si128 ( ( __m128i* ) destBuffer, loaded );
		destBuffer += 8; 
	}
	destBuffer = ( int16_t * ) dest;
	for ( int i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( ( ( int16_t ) srcBuffer [ i ] ) * 256 );
#else
	for ( int i = 0; i < srcByteCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( ( ( int16_t ) srcBuffer [ i ] ) * 256 );
#endif

	return true;
}

// 8-bit PCM data to 24-bit PCM data
extern "C" static inline bool __TC_int8_to_int24 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 3 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	for ( int i = 0; i < srcByteCount; ++i )
	{
		int32_t temp = ( ( int32_t ) srcBuffer [ i ] ) * 65536;
		int8_t _1 = ( int8_t ) ( ( temp >> 24 ) & 0xff ),
			_2 = ( int8_t ) ( ( temp >> 16 ) & 0xff ),
			_3 = ( int8_t ) ( temp & 0xff );
		destBuffer [ 1 ] = _1;
		destBuffer [ 2 ] = _2;
		destBuffer [ 0 ] = _3;
		destBuffer += 3;
	}

	return true;
}

// 8-bit PCM data to 32-bit PCM data
extern "C" static inline bool __TC_int8_to_int32 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int loopCount = srcByteCount / 4 * 4;
	for ( int i = 0; i < loopCount; i += 4 )
	{
		__m128i loaded = _mm_cvtepi8_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) );
		loaded = _mm_mullo_epi32 ( loaded, __g_TC_int8_to_int32_converter );
		_mm_store_si128 ( ( __m128i* ) destBuffer, loaded );
		destBuffer += 4;
	}
	destBuffer = ( int32_t * ) dest;
	for ( int i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( ( ( int32_t ) srcBuffer [ i ] ) * 16777216 );
#else
	for ( int i = 0; i < srcByteCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( ( ( int32_t ) srcBuffer [ i ] ) * 16777216 );
#endif

	return true;
}

// 8-bit PCM data to IEEE 754 Floating point PCM data
extern "C" static inline bool __TC_int8_to_float ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	float * destBuffer = ( float * ) dest;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int loopCount = srcByteCount / 4 * 4;
	for ( int i = 0; i < loopCount; i += 4 )
	{
		__m128 loaded = _mm_cvtepi32_ps ( _mm_cvtepi8_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ) );
		loaded = _mm_mul_ps ( loaded, __g_TC_int8_to_float_converter );
		_mm_store_ps ( destBuffer, loaded );
		destBuffer += 4;
	}
	destBuffer = ( float * ) dest;
	for ( int i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_CHAR );
#else
	for ( int i = 0; i < srcByteCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_CHAR );
#endif

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Converters of 16-bit PCM data to
// 
////////////////////////////////////////////////////////////////////////////////////////////////////

// 16-bit PCM data to 8-bit PCM data
extern "C" static inline bool __TC_int16_to_int8 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 2 )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 2;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_cvtepi16_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) );
		loaded = _mm_srai_epi32 ( loaded, 8 );
		int8_t arr [ 16 ];
		_mm_storel_epi64 ( ( __m128i * ) &arr, _mm_shuffle_epi8 ( loaded, __g_TC_shuffle_int32_to_int8 ) );
		memcpy ( destBuffer, arr, 4 );
		destBuffer += 4;
	}
	destBuffer = ( int8_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] >> 8 );
#else
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] >> 8 );
#endif

	return true;
}

// 16-bit PCM data to 24-bit PCM data
extern "C" static inline bool __TC_int16_to_int24 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount / 2 > destByteSize / 3 )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 2;
	for ( int i = 0; i < loopCount; ++i )
	{
		int32_t temp = ( ( int32_t ) srcBuffer [ i ] ) << 8;
		int8_t _1 = ( int8_t ) ( ( temp >> 24 ) & 0xff ),
			_2 = ( int8_t ) ( ( temp >> 16 ) & 0xff ),
			_3 = ( int8_t ) ( temp & 0xff );
		destBuffer [ 1 ] = _1;
		destBuffer [ 2 ] = _2;
		destBuffer [ 0 ] = _3;
		destBuffer += 3;
	}

	return true;
}

// 16-bit PCM data to 32-bit PCM data
extern "C" static inline bool __TC_int16_to_int32 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int loopCount = srcByteCount / 2;
#if ( /*AE_ARCH_IA32 || */AE_ARCH_AMD64 ) && USE_SIMD		//< Faster only in 64-bit Process. Tested on AMD Ryzen 5 2600X.
															//  SIMD faster than Plain C++ in Debug mode 32-bit Process.
															//  but Release mode 32-bit is slower.
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_cvtepi16_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) );
		loaded = _mm_mul_epi32 ( loaded, __g_TC_int16_to_int32_converter );
		//loaded = _mm_srai_epi32 ( loaded, 16 );
		_mm_store_si128 ( ( __m128i* ) destBuffer, loaded );
		destBuffer += 4;
	}
	destBuffer = ( int32_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( srcBuffer [ i ] ) << 16;
#else
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( srcBuffer [ i ] ) << 16;
#endif

	return true;
}

// 16-bit PCM data to 32-bit IEEE 754 Floating point PCM data
extern "C" static inline bool __TC_int16_to_float ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	float * destBuffer = ( float * ) dest;
	int loopCount = srcByteCount / 2;
#if ( /*AE_ARCH_IA32 || */AE_ARCH_AMD64 ) && USE_SIMD		//< Faster only in 64-bit Process. Tested on AMD Ryzen 5 2600X.
															//  SIMD faster than Plain C++ in Debug mode 32-bit Process.
															//  but Release mode 32-bit is slower.
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_cvtepi32_ps ( _mm_cvtepi16_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ) );
		loaded = _mm_mul_ps ( loaded, __g_TC_int16_to_float_converter );
		_mm_store_ps ( destBuffer, loaded );
		destBuffer += 4;
	}
	destBuffer = ( float * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_SHORT );
#else
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_SHORT );
#endif

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Converters of 24-bit PCM data to
// 
////////////////////////////////////////////////////////////////////////////////////////////////////

// 24-bit PCM data to 8-bit PCM data
extern "C" static inline bool __TC_int24_to_int8 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 3 )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	for ( int i = 0; i < srcByteCount; i += 3 )
	{
		int32_t temp = ( ( ( ( int32_t ) srcBuffer [ 1 ] ) << 24 ) & 0xff0000 ) + ( ( ( ( int32_t ) srcBuffer [ 2 ] ) << 16 ) & 0xff00 ) + srcBuffer [ 0 ];
		destBuffer [ i ] = ( int8_t ) ( temp / 65536 );
		srcBuffer += 3;
	}

	return true;
}

// 24-bit PCM data to 16-bit PCM data
extern "C" static inline bool __TC_int24_to_int16 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount / 3 > destByteSize / 2 )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	for ( int i = 0; i < srcByteCount; i += 3 )
	{
		int32_t temp = ( ( ( ( int32_t ) srcBuffer [ 1 ] ) << 24 ) & 0xff0000 ) + ( ( ( ( int32_t ) srcBuffer [ 2 ] ) << 16 ) & 0xff00 ) + srcBuffer [ 0 ];
		destBuffer [ i ] = ( int16_t ) ( temp / 256 );
		srcBuffer += 3;
	}

	return true;
}

// 24-bit PCM data to 32-bit PCM data
extern "C" static inline bool __TC_int24_to_int32 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount / 3 > destByteSize / 4 )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	for ( int i = 0; i < srcByteCount; i += 3 )
	{
		int32_t temp = ( ( ( ( int32_t ) srcBuffer [ 1 ] ) << 24 ) & 0xff0000 ) + ( ( ( ( int32_t ) srcBuffer [ 2 ] ) << 16 ) & 0xff00 ) + srcBuffer [ 0 ];
		destBuffer [ i ] = temp * 256;
		srcBuffer += 3;
	}

	return true;
}

// 24-bit PCM data to 32-bit IEEE 754 Floating point PCM data
extern "C" static inline bool __TC_int24_to_float ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount / 3 > destByteSize / 4 )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	float * destBuffer = ( float * ) dest;
	int loopCount = srcByteCount / 3;
	for ( int i = 0; i < loopCount; ++i )
	{
		int32_t temp = ( ( ( ( int32_t ) srcBuffer [ 1 ] ) << 24 ) & 0xff0000 ) + ( ( ( ( int32_t ) srcBuffer [ 2 ] ) << 16 ) & 0xff00 ) + srcBuffer [ 0 ];
		destBuffer [ i ] = ( float ) ( temp * __TC_INV_24BIT );
		srcBuffer += 3;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Converters of 32-bit PCM data to
// 
////////////////////////////////////////////////////////////////////////////////////////////////////

// 32-bit PCM data to 8-bit PCM data
extern "C" static inline bool __TC_int32_to_int8 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 4 )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 4;

#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) );
		loaded = _mm_srai_epi32 ( loaded, 24 );
		int8_t arr [ 16 ];
		_mm_storel_epi64 ( ( __m128i * ) &arr, _mm_shuffle_epi8 ( loaded, __g_TC_shuffle_int32_to_int8 ) );
		memcpy ( destBuffer, arr, 4 );
		destBuffer += 4;
	}
	destBuffer = ( int8_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] >> 24 );
#else
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] >> 24 );
#endif

	return true;
}

// 32-bit PCM data to 16-bit PCM data
extern "C" static inline bool __TC_int32_to_int16 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 4 )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int loopCount = srcByteCount / 4;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) );
		loaded = _mm_srai_epi32 ( loaded, 16 );
		int16_t arr [ 8 ];
		_mm_storel_epi64 ( ( __m128i * ) &arr, _mm_packs_epi32 ( loaded, __g_TC_zero_int ) );
		memcpy ( destBuffer, arr, 8 );
		destBuffer += 4;
	}
	destBuffer = ( int16_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( srcBuffer [ i ] >> 16 );
#else
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( srcBuffer [ i ] >> 16 );
#endif

	return true;
}

// 32-bit PCM data to 24-bit PCM data
extern "C" static inline bool __TC_int32_to_int24 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 4 )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 4;
	for ( int i = 0; i < loopCount; ++i )
	{
		int8_t _1 = ( int8_t ) ( ( srcBuffer [ i ] >> 24 ) & 0xff ),
			_2 = ( int8_t ) ( ( srcBuffer [ i ] >> 16 ) & 0xff ),
			_3 = ( int8_t ) ( srcBuffer [ i ] & 0xff );
		destBuffer [ 1 ] = _1;
		destBuffer [ 2 ] = _2;
		destBuffer [ 0 ] = _3;
		destBuffer += 3;
	}

	return true;
}

// 32-bit PCM data to 32-bit IEEE 754 Floating point PCM data
extern "C" static inline bool __TC_int32_to_float ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	float * destBuffer = ( float * ) dest;
	int loopCount = srcByteCount / 4;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_cvtepi32_ps ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ) );
		loaded = _mm_mul_ps ( loaded, __g_TC_int32_to_float_converter );
		_mm_store_ps ( destBuffer, loaded );
		destBuffer += 4;
	}
	destBuffer = ( float * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_INT );
#else
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_INT );
#endif

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Converters of 32-bit IEEE 754 Floating point data to
// 
////////////////////////////////////////////////////////////////////////////////////////////////////

// 32-bit IEEE 754 Floating point PCM data to 8-bit PCM data
extern "C" static inline bool __TC_float_to_int8 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 4 )
		return false;

	float * srcBuffer = ( float * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 4;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_load_ps ( srcBuffer + i );
		loaded = _mm_mul_ps ( _mm_min_ps ( __g_TC_min_value, _mm_max_ps ( __g_TC_max_value, loaded ) ), __g_TC_float_to_int8_converter );
		int8_t arr [ 16 ];
		_mm_storel_epi64 ( ( __m128i * ) &arr, _mm_shuffle_epi8 ( _mm_cvtps_epi32 ( loaded ), __g_TC_shuffle_int32_to_int8 ) );
		memcpy ( destBuffer, arr, 4 );
		destBuffer += 4;
	}
	destBuffer = ( int8_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] * 128 );
#else
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] * 128 );
#endif

	return true;
}

// 32-bit IEEE 754 Floating point PCM data to 16-bit PCM data
extern "C" static inline bool __TC_float_to_int16 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 2 )
		return false;

	float * srcBuffer = ( float * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int loopCount = srcByteCount / 4;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_load_ps ( srcBuffer + i );
		loaded = _mm_mul_ps ( _mm_min_ps ( __g_TC_min_value, _mm_max_ps ( __g_TC_max_value, loaded ) ), __g_TC_float_to_int16_converter );
		int16_t arr [ 8 ];
		_mm_storel_epi64 ( ( __m128i * ) &arr, _mm_packs_epi32 ( _mm_cvtps_epi32 ( loaded ), __g_TC_zero_int ) );
		memcpy ( destBuffer, arr, 8 );
		destBuffer += 4;
	}
	destBuffer = ( int16_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( min ( 1, max ( -1, srcBuffer [ i ] ) ) * SHRT_MAX );
#else
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( min ( 1, max ( -1, srcBuffer [ i ] ) ) * SHRT_MAX );
#endif

	return true;
}

// 32-bit IEEE 754 Floating point PCM data to 24-bit PCM data
extern "C" static inline bool __TC_float_to_int24 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount / 4 > destByteSize / 3 )
		return false;

	float * srcBuffer = ( float * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 4;
	for ( int i = 0; i < loopCount; ++i )
	{
		int32_t temp = ( int32_t ) ( min ( 1, max ( -1, srcBuffer [ i ] ) ) * __TC_24BIT );
		int8_t _1 = ( int8_t ) ( ( temp >> 24 ) & 0xff ),
			_2 = ( int8_t ) ( ( temp >> 16 ) & 0xff ),
			_3 = ( int8_t ) ( temp & 0xff );
		destBuffer [ 1 ] = _1;
		destBuffer [ 2 ] = _2;
		destBuffer [ 0 ] = _3;
		destBuffer += 3;
	}

	return true;
}

// 32-bit IEEE 754 Floating point PCM data to 32-bit PCM data
extern "C" static inline bool __TC_float_to_int32 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize )
		return false;

	float * srcBuffer = ( float * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int loopCount = srcByteCount / 4;
#if ( /*AE_ARCH_IA32 || */AE_ARCH_AMD64 ) && USE_SIMD		//< Faster only in 64-bit Process. Tested on AMD Ryzen 5 2600X.
															//  SIMD faster than Plain C++ in Debug mode 32-bit Process.
															//  but Release mode 32-bit is slower.
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_load_ps ( srcBuffer + i );
		loaded = _mm_mul_ps ( _mm_min_ps ( __g_TC_min_value, _mm_max_ps ( __g_TC_max_value, loaded ) ), __g_TC_float_to_int32_converter );
		_mm_store_si128 ( ( __m128i * ) ( destBuffer + i ), _mm_cvtps_epi32 ( loaded ) );
	}
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( min ( 1, max ( -1, srcBuffer [ i ] ) ) * INT_MAX );
#else
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( min ( 1, max ( -1, srcBuffer [ i ] ) ) * INT_MAX );
#endif

	return true;
}

#endif