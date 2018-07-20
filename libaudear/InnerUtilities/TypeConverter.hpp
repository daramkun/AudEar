#ifndef __AUDEAR_TYPE_CONVERTER_HPP__
#define __AUDEAR_TYPE_CONVERTER_HPP__

#include <cstdint>
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
#	include <mmintrin.h>
#	include <emmintrin.h>
#	include <immintrin.h>
#	include <pmmintrin.h>
#	include <smmintrin.h>
#	include <xmmintrin.h>
#endif

#define __TC_INV_CHAR										0.0078125
#define __TC_INV_SHORT										3.0518509475997192297128208258309e-5
#define __TC_INV_24BIT										1.1920930376163765926810017443897e-7
#define __TC_INV_INT										4.656612875245796924105750827168e-10

#if USE_PARALLEL
#	define __TC_PARALLEL									#pragma omp parallel for
#else
#	define __TC_PARALLEL
#endif

typedef bool ( *__TypeConverterFunction ) ( void * src, void * dest, int srcByteCount, int destByteSize );

static inline bool __TC_int8_to_int16 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 2 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int16_t * destBuffer = ( int16_t * ) src;
	for ( int i = 0; i < srcByteCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( ( ( int16_t ) srcBuffer [ i ] ) * 256 );

	return true;
}

static inline bool __TC_int8_to_int24 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 3 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int8_t * destBuffer = ( int8_t * ) src;
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

static inline bool __TC_int8_to_int32 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int32_t * destBuffer = ( int32_t * ) src;
	for ( int i = 0; i < srcByteCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( ( ( int32_t ) srcBuffer [ i ] ) * 16777216 );

	return true;
}

static inline bool __TC_int8_to_float ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	static __m128 converter = _mm_set1_ps ( __TC_INV_CHAR );
#endif

	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	float * destBuffer = ( float * ) dest;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int loopCount = srcByteCount / 4 * 4;
	for ( int i = 0; i < loopCount; i += 4 )
	{
		__m128 loaded = _mm_setr_ps ( srcBuffer [ i ], srcBuffer [ i + 1 ], srcBuffer [ i + 2 ], srcBuffer [ i + 3 ] );
		loaded = _mm_mul_ps ( loaded, converter );
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

static inline bool __TC_int16_to_int8 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 2 )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int8_t * destBuffer = ( int8_t * ) src;
	int loopCount = srcByteCount / 2;
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] / 256 );

	return true;
}

static inline bool __TC_int16_to_int24 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount / 2 > destByteSize / 3 )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int8_t * destBuffer = ( int8_t * ) src;
	int loopCount = srcByteCount / 2;
	for ( int i = 0; i < loopCount; ++i )
	{
		int32_t temp = ( ( int32_t ) srcBuffer [ i ] ) * 256;
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

static inline bool __TC_int16_to_int32 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int32_t * destBuffer = ( int32_t * ) src;
	int loopCount = srcByteCount / 2;
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( srcBuffer [ i ] ) * 65536;

	return true;
}

static inline bool __TC_int16_to_float ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	static __m128 converter = _mm_set1_ps ( __TC_INV_SHORT );
#endif

	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	float * destBuffer = ( float * ) dest;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int loopCount = srcByteCount / 2;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_setr_ps ( srcBuffer [ i ], srcBuffer [ i + 1 ], srcBuffer [ i + 2 ], srcBuffer [ i + 3 ] );
		loaded = _mm_mul_ps ( loaded, converter );
		_mm_store_ps ( destBuffer, loaded );
		destBuffer += 4;
	}
	destBuffer = ( float * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_SHORT );
#else
	int loopCount = srcByteCount / 2;
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_SHORT );
#endif

	return true;
}

static inline bool __TC_int24_to_int8 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
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

static inline bool __TC_int24_to_int16 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
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

static inline bool __TC_int24_to_int32 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
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

static inline bool __TC_int24_to_float ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
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

static inline bool __TC_int32_to_int8 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 4 )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 4;
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] / 16777216 );

	return true;
}

static inline bool __TC_int32_to_int16 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount > destByteSize * 4 )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int loopCount = srcByteCount / 4;
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( srcBuffer [ i ] / 65536 );

	return true;
}

static inline bool __TC_int32_to_int24 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
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

static inline bool __TC_int32_to_float ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	static __m128 converter = _mm_set1_ps ( __TC_INV_INT );
#endif

	if ( srcByteCount > destByteSize )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	float * destBuffer = ( float * ) dest;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_setr_ps ( ( float ) srcBuffer [ i ], ( float ) srcBuffer [ i + 1 ], ( float ) srcBuffer [ i + 2 ], ( float ) srcBuffer [ i + 3 ] );
		loaded = _mm_mul_ps ( loaded, converter );
		_mm_store_ps ( destBuffer, loaded );
		destBuffer += 4;
	}
	destBuffer = ( float * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_INT );
#else
	int loopCount = srcByteCount / 2;
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( float ) ( srcBuffer [ i ] * __TC_INV_INT );
#endif

	return true;
}

static inline bool __TC_float_to_int8 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	static __m128 converter = _mm_set1_ps ( 128 );
	static __m128 minVal = _mm_set1_ps ( 1 );
	static __m128 maxVal = _mm_set1_ps ( -1 );
#endif

	if ( srcByteCount > destByteSize * 4 )
		return false;

	float * srcBuffer = ( float * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_setr_ps ( srcBuffer [ i ], srcBuffer [ i + 1 ], srcBuffer [ i + 2 ], srcBuffer [ i + 3 ] );
		loaded = _mm_mul_ps ( _mm_min_ps ( minVal, _mm_max_ps ( maxVal, loaded ) ), converter );
		float arr [ 4 ];
		_mm_store_ps ( arr, loaded );
		destBuffer [ 0 ] = ( int8_t ) arr [ 0 ];
		destBuffer [ 1 ] = ( int8_t ) arr [ 1 ];
		destBuffer [ 2 ] = ( int8_t ) arr [ 2 ];
		destBuffer [ 3 ] = ( int8_t ) arr [ 3 ];
		destBuffer += 4;
	}
	destBuffer = ( int8_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] * 128 );
#else
	int loopCount = srcByteCount / 4;
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int8_t ) ( srcBuffer [ i ] * 128 );
#endif

	return true;
}

static inline bool __TC_float_to_int16 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	static __m128 converter = _mm_set1_ps ( SHRT_MAX );
	static __m128 minVal = _mm_set1_ps ( 1 );
	static __m128 maxVal = _mm_set1_ps ( -1 );
#endif

	if ( srcByteCount > destByteSize * 2 )
		return false;

	float * srcBuffer = ( float * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_setr_ps ( srcBuffer [ i ], srcBuffer [ i + 1 ], srcBuffer [ i + 2 ], srcBuffer [ i + 3 ] );
		loaded = _mm_mul_ps ( _mm_min_ps ( minVal, _mm_max_ps ( maxVal, loaded ) ), converter );
		float arr [ 4 ];
		_mm_store_ps ( arr, loaded );
		destBuffer [ 0 ] = ( int16_t ) arr [ 0 ];
		destBuffer [ 1 ] = ( int16_t ) arr [ 1 ];
		destBuffer [ 2 ] = ( int16_t ) arr [ 2 ];
		destBuffer [ 3 ] = ( int16_t ) arr [ 3 ];
		destBuffer += 4;
	}
	destBuffer = ( int16_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( srcBuffer [ i ] * SHRT_MAX );
#else
	int loopCount = srcByteCount / 4;
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int16_t ) ( srcBuffer [ i ] * SHRT_MAX );
#endif

	return true;
}

static inline bool __TC_float_to_int24 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount / 4 > destByteSize / 3 )
		return false;

	float * srcBuffer = ( float * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 4;
	for ( int i = 0; i < loopCount; ++i )
	{
		int32_t temp = ( int32_t ) ( srcBuffer [ i ] * 8388607 );
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

static inline bool __TC_float_to_int32 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	static __m128 converter = _mm_set1_ps ( ( float ) INT_MAX );
	static __m128 minVal = _mm_set1_ps ( 1 );
	static __m128 maxVal = _mm_set1_ps ( -1 );
#endif

	if ( srcByteCount > destByteSize )
		return false;

	float * srcBuffer = ( float * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 ) && USE_SIMD
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loaded = _mm_setr_ps ( srcBuffer [ i ], srcBuffer [ i + 1 ], srcBuffer [ i + 2 ], srcBuffer [ i + 3 ] );
		loaded = _mm_mul_ps ( _mm_min_ps ( minVal, _mm_max_ps ( maxVal, loaded ) ), converter );
		float arr [ 4 ];
		_mm_store_ps ( arr, loaded );
		destBuffer [ 0 ] = ( int32_t ) arr [ 0 ];
		destBuffer [ 1 ] = ( int32_t ) arr [ 1 ];
		destBuffer [ 2 ] = ( int32_t ) arr [ 2 ];
		destBuffer [ 3 ] = ( int32_t ) arr [ 3 ];
		destBuffer += 4;
	}
	destBuffer = ( int32_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( srcBuffer [ i ] * INT_MAX );
#else
	int loopCount = srcByteCount / 4;
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( srcBuffer [ i ] * INT_MAX );
#endif

	return true;
}

#endif