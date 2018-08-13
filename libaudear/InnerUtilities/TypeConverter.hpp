#ifndef __AUDEAR_TYPE_CONVERTER_HPP__
#define __AUDEAR_TYPE_CONVERTER_HPP__

#pragma warning ( disable: 4068 )

/***************************************************************************************************
*
* TypeConverter.hpp
*   - 2018-07-20 Jin Jae-yeon
*   - Functions module for PCM audio data converter
*
***************************************************************************************************/

#include <cstdint>
#include <cstring>
#include <cmath>
#include <type_traits>

#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 )
//  MMX, SSE, SSE 2
#	include <mmintrin.h>
#	include <xmmintrin.h>
#	include <emmintrin.h>
//  SSE 3
#	include <pmmintrin.h>
//  SSE 4.x
#	include <smmintrin.h>
#	include <nmmintrin.h>
//  AVX
#	include <immintrin.h>
#elif ( AE_ARCH_ARM || AE_ARCH_ARM64 )
//  NEON
#	include <arm_neon.h>
#	if AE_PLATFORM_ANDROID
#		include <cpu-features.h>
#	endif
#endif

#pragma pack ( push, 1 )
struct int24_t
{
	uint8_t value [ 3 ];

	int24_t () = default;
	inline int24_t ( int32_t v ) { memcpy ( value, &v, 3 ); }
	inline operator int32_t () const
	{
		//return ( ( value [ 2 ] & 0x80 ) ? ( 0xff000000 ) : 0 )
		return ( ( value [ 2 ] & 0x80 ) << 24 ) >> 7
			| ( value [ 2 ] << 16 ) | ( value [ 1 ] << 8 ) | value [ 0 ];
	}
};
#pragma pack ( pop )

#define __TC_CHAR											127.0f
#define __TC_SHORT											32767.0f
#define __TC_24BIT											8388607.0f
#define __TC_INT											2147483647.0

#define __TC_INV_CHAR										0.0078125f
#define __TC_INV_SHORT										3.0518509475997192297128208258309e-5f
#define __TC_INV_24BIT										1.1920930376163765926810017443897e-7f
#define __TC_INV_INT										4.656612875245796924105750827168e-10

#define SSE_CONVERT_COUNT_UNIT								0xfffffffc								//< ~4 + 1  = -4
#define AVX_CONVERT_COUNT_UNIT								0xfffffff8								//< ~8 + 1  = -8
#define AVX512_CONVERT_COUNT_UNIT							0xfffffff0								//< ~16 + 1 = -16

#pragma region	Plain Type Convert Inline functions
#pragma mark	Plain Type Convert Inline functions
template<typename S, typename D>
static inline D __TC_convert ( S value ) { static_assert ( true, "Not supported format." ); }
template<> static inline float __TC_convert ( int8_t value ) { return ( float ) ( min ( 1, max ( -1, value * __TC_INV_CHAR ) ) ); }
template<> static inline float __TC_convert ( int16_t value ) { return ( float ) ( min ( 1, max ( -1, value * __TC_INV_SHORT ) ) ); }
template<> static inline float __TC_convert ( int24_t value ) { return ( float ) ( min ( 1, max ( -1, ( ( int32_t ) value ) * __TC_INV_24BIT ) ) ); }
template<> static inline float __TC_convert ( int32_t value ) { return ( float ) ( min ( 1, max ( -1, value * __TC_INV_INT ) ) ); }
template<> static inline int8_t __TC_convert ( float value ) { return ( int8_t ) ( min ( 1, max ( -1, value ) ) * __TC_CHAR ); }
template<> static inline int16_t __TC_convert ( float value ) { return ( int16_t ) ( min ( 1, max ( -1, value ) ) * __TC_SHORT ); }
template<> static inline int24_t __TC_convert ( float value ) { return ( int24_t ) ( int32_t ) ( min ( 1, max ( -1, value ) ) * __TC_24BIT ); }
template<> static inline int32_t __TC_convert ( float value ) { return ( int32_t ) ( min ( 1, max ( -1, value ) ) * __TC_INT ); }
template<> static inline int16_t __TC_convert ( int8_t value ) { return value << 8; }
template<> static inline int24_t __TC_convert ( int8_t value ) { return ( int24_t ) ( ( ( int32_t ) value ) << 16 ); }
template<> static inline int32_t __TC_convert ( int8_t value ) { return value << 24; }
template<> static inline int8_t __TC_convert ( int16_t value ) { return value >> 8; }
template<> static inline int24_t __TC_convert ( int16_t value ) { return ( int24_t ) ( value << 8 ); }
template<> static inline int32_t __TC_convert ( int16_t value ) { return value << 16; }
template<> static inline int8_t __TC_convert ( int24_t value ) { return ( ( int32_t ) value ) >> 16; }
template<> static inline int16_t __TC_convert ( int24_t value ) { return ( ( int32_t ) value ) >> 8; }
template<> static inline int32_t __TC_convert ( int24_t value ) { return ( ( int32_t ) value ) << 8; }
template<> static inline int8_t __TC_convert ( int32_t value ) { return value >> 24; }
template<> static inline int16_t __TC_convert ( int32_t value ) { return value >> 16; }
template<> static inline int24_t __TC_convert ( int32_t value ) { return ( int24_t ) ( value >> 8 ); }
#pragma endregion

#pragma region	Type Converters Selector
#pragma mark	Type Converters Selector
typedef bool ( *__TypeConverterFunction ) ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize );
struct __TC_SAMPLE_CONVERTERS
{
	__TypeConverterFunction  f32_to_i8, f32_to_i16, f32_to_i24, f32_to_i32;
	__TypeConverterFunction  i8_to_f32,  i8_to_i16,  i8_to_i24,  i8_to_i32;
	__TypeConverterFunction i16_to_f32,  i16_to_i8, i16_to_i24, i16_to_i32;
	__TypeConverterFunction i24_to_f32,  i24_to_i8, i24_to_i16, i24_to_i32;
	__TypeConverterFunction i32_to_f32,  i32_to_i8, i32_to_i16, i32_to_i24;

	template<typename S, typename D>
	inline __TypeConverterFunction get_converter () const
	{
		if ( std::is_same<S, float>::value )
		{
			if ( std::is_same<D, int16_t>::value ) return f32_to_i16;
			else if ( std::is_same<D, int24_t>::value ) return f32_to_i24;
			else if ( std::is_same<D, int32_t>::value ) return f32_to_i32;
			else if ( std::is_same<D, int8_t>::value ) return f32_to_i8;
		}
		else if ( std::is_same<S, int16_t>::value )
		{
			if ( std::is_same<D, float>::value ) return i16_to_f32;
			else if ( std::is_same<D, int24_t>::value ) return i16_to_i24;
			else if ( std::is_same<D, int32_t>::value ) return i16_to_i32;
			else if ( std::is_same<D, int8_t>::value ) return i16_to_i8;
		}
		else if ( std::is_same<S, int24_t>::value )
		{
			if ( std::is_same<D, float>::value ) return i24_to_f32;
			else if ( std::is_same<D, int16_t>::value ) return i24_to_i16;
			else if ( std::is_same<D, int32_t>::value ) return i24_to_i32;
			else if ( std::is_same<D, int8_t>::value ) return i24_to_i8;
		}
		else if ( std::is_same<S, int32_t>::value )
		{
			if ( std::is_same<D, float>::value ) return i32_to_f32;
			else if ( std::is_same<D, int16_t>::value ) return i32_to_i16;
			else if ( std::is_same<D, int24_t>::value ) return i32_to_i24;
			else if ( std::is_same<D, int8_t>::value ) return i32_to_i8;
		}
		else if ( std::is_same<S, int8_t>::value )
		{
			if ( std::is_same<D, float>::value ) return i8_to_f32;
			else if ( std::is_same<D, int16_t>::value ) return i8_to_i16;
			else if ( std::is_same<D, int24_t>::value ) return i8_to_i24;
			else if ( std::is_same<D, int32_t>::value ) return i8_to_i32;
		}
		static_assert ( true, "Not supported format." );
		return nullptr;
	}
};
#pragma endregion

#pragma region	Plain Type Converter
#pragma mark	Plain Type Converter
template <typename S, typename D>
static inline bool __TC_sample_convert ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	if ( srcByteCount * sizeof ( D ) > destByteSize * sizeof ( S ) )
		return false;

	S * srcBuffer = ( S* ) src;
	D * destBuffer = ( D* ) dest;
	int64_t loopCount = srcByteCount / sizeof ( S );
	for ( int i = 0; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<S, D> ( srcBuffer [ i ] );

	return true;
}

static const __TC_SAMPLE_CONVERTERS __g_TC_plain_converters = {
	__TC_sample_convert<float, int8_t>,
	__TC_sample_convert<float, int16_t>,
	__TC_sample_convert<float, int24_t>,
	__TC_sample_convert<float, int32_t>,

	__TC_sample_convert<int8_t, float>,
	__TC_sample_convert<int8_t, int16_t>,
	__TC_sample_convert<int8_t, int24_t>,
	__TC_sample_convert<int8_t, int32_t>,

	__TC_sample_convert<int16_t, float>,
	__TC_sample_convert<int16_t, int8_t>,
	__TC_sample_convert<int16_t, int24_t>,
	__TC_sample_convert<int16_t, int32_t>,

	__TC_sample_convert<int24_t, float>,
	__TC_sample_convert<int24_t, int8_t>,
	__TC_sample_convert<int24_t, int16_t>,
	__TC_sample_convert<int24_t, int32_t>,

	__TC_sample_convert<int32_t, float>,
	__TC_sample_convert<int32_t, int8_t>,
	__TC_sample_convert<int32_t, int16_t>,
	__TC_sample_convert<int32_t, int24_t>,
};

static inline bool __TC_plain_support () {
	return true;
}
#pragma endregion

#pragma region	SSE Type Converter
#pragma mark	SSE Type Converter
#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 )
////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//  //////////  //////////  //////////    //////////  //////////  //      //  //      //          //
//  //          //          //            //          //      //  ////    //  //      //          //
//  //          //          //            //          //      //  //  //  //   //    //           //
//  //////////  //////////  //////////    //          //      //  //  //  //   //    //           //
//          //          //  //            //          //      //  //  //  //    //  //            //
//          //          //  //            //          //      //  //    ////    //  //            //
//  //////////  //////////  //////////    //////////  //////////  //      //      //              //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////
static const __m128 __g_TC_min_value = _mm_set1_ps ( 1 );
static const __m128 __g_TC_max_value = _mm_set1_ps ( -1 );

static inline __m128 __TC_convert ( __m128i value, __m128 conv )
{
	return _mm_min_ps ( __g_TC_min_value, _mm_max_ps ( __g_TC_max_value, _mm_mul_ps ( _mm_cvtepi32_ps ( value ), conv ) ) );
}
static inline __m128i __TC_convert ( __m128 value, __m128 conv )
{
	return _mm_cvtps_epi32 ( _mm_mul_ps ( ( _mm_min_ps ( __g_TC_min_value, _mm_max_ps ( __g_TC_max_value, value ) ) ), conv ) );
}

template<typename S, typename D>
static inline bool __TC_sample_convert_sse ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static_assert ( true, "Not supported format." );
	return false;
}

#	pragma region Floating point to
#	pragma mark - Floating point to
////////////////////////////////////////////////////////////
// float to
////////////////////////////////////////////////////////////
template<> static inline bool __TC_sample_convert_sse<float, int8_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128 f32_to_i8_converter = _mm_set1_ps ( ( float ) __TC_CHAR );
	static const __m128i i32_to_i8_shuffle = _mm_set_epi8 ( -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0 );

	if ( srcByteCount > destByteSize * 4 )
		return false;

	int8_t arr [ 16 ];

	float * srcBuffer = ( float * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( float );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128 loadedf = _mm_load_ps ( srcBuffer + i );
		__m128i loaded = __TC_convert ( loadedf, f32_to_i8_converter );
		loaded = _mm_shuffle_epi8 ( loaded, i32_to_i8_shuffle );
		_mm_store_si128 ( ( __m128i* ) arr, loaded );
		memcpy ( destBuffer + i, arr, 4 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int8_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<float, int16_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128  f32_to_i16_converter = _mm_set1_ps ( ( float ) __TC_SHORT );
	static const __m128i zero_int = _mm_setzero_si128 ();

	if ( srcByteCount > destByteSize * 2 )
		return false;

	int16_t arr [ 8 ];

	float * srcBuffer = ( float * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( float );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = __TC_convert ( _mm_load_ps ( srcBuffer + i ), f32_to_i16_converter );
		_mm_storel_epi64 ( ( __m128i * ) &arr, _mm_packs_epi32 ( loaded, zero_int ) );
		memcpy ( destBuffer + i, arr, 8 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int16_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<float, int24_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128 f32_to_i24_converter = _mm_set1_ps ( ( float ) __TC_24BIT );
	static const __m128i i32_to_i24_shuffle = _mm_set_epi8 ( -1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0 );

	if ( srcByteCount * 3 > destByteSize * 4 )
		return false;

	int8_t arr [ 16 ];

	float * srcBuffer = ( float * ) src;
	int24_t * destBuffer = ( int24_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( float );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = __TC_convert ( _mm_load_ps ( srcBuffer + i ), f32_to_i24_converter );
		loaded = _mm_shuffle_epi8 ( loaded, i32_to_i24_shuffle );
		_mm_store_si128 ( ( __m128i * ) &arr, loaded );
		memcpy ( destBuffer + i, arr, 12 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int24_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<float, int32_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128 f32_to_i32_converter = _mm_set1_ps ( ( float ) __TC_INT );

	if ( srcByteCount > destByteSize )
		return false;

	float * srcBuffer = ( float * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( float );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
		_mm_store_si128 ( ( __m128i * ) ( destBuffer + i ), __TC_convert ( _mm_load_ps ( srcBuffer + i ), f32_to_i32_converter ) );
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int32_t> ( srcBuffer [ i ] );

	return true;
}
#	pragma endregion

#	pragma region 8-bit Integer to
#	pragma mark - 8-bit Integer to
////////////////////////////////////////////////////////////
// 8-bit to
////////////////////////////////////////////////////////////
template<> static inline bool __TC_sample_convert_sse<int8_t, int16_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i zero_int = _mm_setzero_si128 ();

	if ( srcByteCount * 2 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int64_t loopCount = srcByteCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < loopCount; i += 8 )
	{
		__m128i loaded = _mm_slli_epi16 ( _mm_cvtepi8_epi16 ( _mm_loadl_epi64 ( ( __m128i* ) ( srcBuffer + i ) ) ), 8 );
		_mm_storel_epi64 ( ( __m128i* ) ( destBuffer + i ), loaded );
	}
	for ( int64_t i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = __TC_convert<int8_t, int16_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int8_t, int24_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i i32_to_i24_shuffle = _mm_set_epi8 ( -1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0 );

	if ( srcByteCount * 3 > destByteSize )
		return false;

	int8_t arr [ 16 ];

	int8_t * srcBuffer = ( int8_t * ) src;
	int24_t * destBuffer = ( int24_t * ) dest;
	int64_t loopCount = srcByteCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < loopCount; i += 4 )
	{
		__m128i loaded = _mm_slli_epi32 ( _mm_cvtepi8_epi32 ( _mm_loadl_epi64 ( ( __m128i* ) ( srcBuffer + i ) ) ), 16 );
		_mm_store_si128 ( ( __m128i* ) arr, _mm_shuffle_epi8 ( loaded, i32_to_i24_shuffle ) );
		memcpy ( destBuffer + i, arr, 12 );
	}
	for ( int64_t i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = __TC_convert<int8_t, int32_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int8_t, int32_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int64_t loopCount = srcByteCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < loopCount; i += 4 )
	{
		__m128i loaded = _mm_slli_epi32 ( _mm_cvtepi8_epi32 ( _mm_loadl_epi64 ( ( __m128i* ) ( srcBuffer + i ) ) ), 24 );
		_mm_store_si128 ( ( __m128i* ) ( destBuffer + 4 ), loaded );
	}
	for ( int64_t i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = __TC_convert<int8_t, int32_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int8_t, float> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128  i8_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_CHAR );
	
	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	float * destBuffer = ( float * ) dest;
	int64_t loopCount = srcByteCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < loopCount; i += 4 )
	{
		__m128 loaded = __TC_convert ( _mm_cvtepi8_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ), i8_to_f32_converter );
		_mm_store_ps ( destBuffer + i, loaded );
	}
	for ( int64_t i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = __TC_convert<int8_t, float> ( srcBuffer [ i ] );

	return true;
}
#	pragma endregion

#	pragma region 16-bit Integer to
#	pragma mark - 16-bit Integer to
////////////////////////////////////////////////////////////
// 16-bit to
////////////////////////////////////////////////////////////
template<> static inline bool __TC_sample_convert_sse<int16_t, int8_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i i16_to_i8_shuffle = _mm_set_epi8 ( -1, -1, -1, -1, -1, -1, -1, -1, 28, 24, 20, 16, 12, 8, 4, 0 );

	if ( srcByteCount > destByteSize * 2 )
		return false;

	int8_t arr [ 16 ];

	int16_t * srcBuffer = ( int16_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int16_t );
	int64_t SIMDLoopCount = loopCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
	{
		__m128i loaded = _mm_srai_epi16 ( _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) ), 8 );
		_mm_store_si128 ( ( __m128i* )arr, _mm_shuffle_epi8 ( loaded, i16_to_i8_shuffle ) );
		memcpy ( destBuffer + i, arr, 8 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, int8_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int16_t, int24_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i i32_to_i24_shuffle = _mm_set_epi8 ( -1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0 );
	
	if ( srcByteCount * 3 > destByteSize * 2 )
		return false;

	int8_t temp [ 16 ];

	int16_t * srcBuffer = ( int16_t * ) src;
	int24_t * destBuffer = ( int24_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int16_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_slli_epi32 ( _mm_cvtepi16_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ), 8 );
		_mm_store_si128 ( ( __m128i* ) temp, _mm_shuffle_epi8 ( loaded, i32_to_i24_shuffle ) );
		memcpy ( destBuffer + i, temp, 12 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, int24_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int16_t, int32_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int16_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_slli_epi32 ( _mm_cvtepi16_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ), 16 );
		_mm_store_si128 ( ( __m128i* ) ( destBuffer + i ), loaded );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, int32_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int16_t, float> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128  i16_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_SHORT );

	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	float * destBuffer = ( float * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int16_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
		_mm_store_ps ( destBuffer + i, __TC_convert ( _mm_cvtepi16_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ), i16_to_f32_converter ) );
	destBuffer = ( float * ) dest;
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, float> ( srcBuffer [ i ] );

	return true;
}
#	pragma endregion

#	pragma region 24-bit Integer to
#	pragma mark - 24-bit Integer to
////////////////////////////////////////////////////////////
// 24-bit to
////////////////////////////////////////////////////////////
template<> static inline bool __TC_sample_convert_sse<int24_t, int8_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i i32_to_i8_shuffle = _mm_set_epi8 ( -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0 );
	static const __m128i i24_to_i32_shuffle = _mm_set_epi8 ( -1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0 );
	static const __m128i i24_to_i32_negate_shuffle = _mm_set_epi8 ( 14, -1, -1, -1, 10, -1, -1, -1, 6, -1, -1, -1, 2, -1, -1, -1 );
	static const __m128i i24_to_i32_and = _mm_set1_epi32 ( ( int ) 0x80000000 );

	if ( srcByteCount > destByteSize * 3 )
		return false;

	int8_t arr [ 16 ];

	int24_t * srcBuffer = ( int24_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int24_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_shuffle_epi8 ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ), i24_to_i32_shuffle );
		__m128i negate = _mm_srai_epi32 ( _mm_and_si128 ( _mm_shuffle_epi8 ( loaded, i24_to_i32_negate_shuffle ), i24_to_i32_and ), 7 );
		loaded = _mm_srai_epi32 ( _mm_or_si128 ( loaded, negate ), 16 );
		_mm_store_si128 ( ( __m128i* ) arr, _mm_shuffle_epi8 ( loaded, i32_to_i8_shuffle ) );
		memcpy ( destBuffer + i, arr, 4 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int24_t, int8_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int24_t, int16_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i zero_int = _mm_setzero_si128 ();
	static const __m128i i24_to_i32_shuffle = _mm_set_epi8 ( -1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0 );
	static const __m128i i24_to_i32_negate_shuffle = _mm_set_epi8 ( 14, -1, -1, -1, 10, -1, -1, -1, 6, -1, -1, -1, 2, -1, -1, -1 );
	static const __m128i i24_to_i32_and = _mm_set1_epi32 ( ( int ) 0x80000000 );

	if ( srcByteCount * 2 > destByteSize * 3 )
		return false;

	int16_t temp_arr [ 8 ];

	int24_t * srcBuffer = ( int24_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int24_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_shuffle_epi8 ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ), i24_to_i32_shuffle );
		__m128i negate = _mm_srai_epi32 ( _mm_and_si128 ( _mm_shuffle_epi8 ( loaded, i24_to_i32_negate_shuffle ), i24_to_i32_and ), 7 );
		loaded = _mm_srai_epi32 ( _mm_or_si128 ( loaded, negate ), 8 );
		_mm_storel_epi64 ( ( __m128i * ) temp_arr, _mm_packs_epi32 ( loaded, zero_int ) );
		memcpy ( destBuffer + i, temp_arr, 8 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int24_t, int16_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int24_t, int32_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i i24_to_i32_shuffle = _mm_set_epi8 ( -1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0 );
	static const __m128i i24_to_i32_negate_shuffle = _mm_set_epi8 ( 14, -1, -1, -1, 10, -1, -1, -1, 6, -1, -1, -1, 2, -1, -1, -1 );
	static const __m128i i24_to_i32_and = _mm_set1_epi32 ( ( int ) 0x80000000 );

	if ( srcByteCount * 4 > destByteSize * 3 )
		return false;

	int24_t * srcBuffer = ( int24_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int24_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_shuffle_epi8 ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ), i24_to_i32_shuffle );
		__m128i negate = _mm_srai_epi32 ( _mm_and_si128 ( _mm_shuffle_epi8 ( loaded, i24_to_i32_negate_shuffle ), i24_to_i32_and ), 7 );
		loaded = _mm_slli_epi32 ( _mm_or_si128 ( loaded, negate ), 8 );
		_mm_store_si128 ( ( __m128i* ) ( destBuffer + i ), loaded );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int24_t, int32_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int24_t, float> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128  i24_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_24BIT );
	static const __m128i i24_to_i32_shuffle = _mm_set_epi8 ( -1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0 );
	static const __m128i i24_to_i32_negate_shuffle = _mm_set_epi8 ( 14, -1, -1, -1, 10, -1, -1, -1, 6, -1, -1, -1, 2, -1, -1, -1 );
	static const __m128i i24_to_i32_and = _mm_set1_epi32 ( ( int ) 0x80000000 );

	if ( srcByteCount * 4 > destByteSize * 3 )
		return false;

	int24_t * srcBuffer = ( int24_t * ) src;
	float * destBuffer = ( float * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int24_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_shuffle_epi8 ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ), i24_to_i32_shuffle );
		__m128i negate = _mm_srai_epi32 ( _mm_and_si128 ( _mm_shuffle_epi8 ( loaded, i24_to_i32_negate_shuffle ), i24_to_i32_and ), 7 );
		loaded = _mm_or_si128 ( loaded, negate );
		__m128 conved = __TC_convert ( loaded, i24_to_f32_converter );
		_mm_store_ps ( destBuffer + i, conved );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int24_t, float> ( srcBuffer [ i ] );

	return true;
}
#	pragma endregion

#	pragma region 32-bit Integer to
#	pragma mark - 32-bit Integer to
////////////////////////////////////////////////////////////
// 32-bit to
////////////////////////////////////////////////////////////
template<> static inline bool __TC_sample_convert_sse<int32_t, int8_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i i32_to_i8_shuffle = _mm_set_epi8 ( -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0 );
	
	if ( srcByteCount > destByteSize * 4 )
		return false;

	int8_t arr [ 16 ];

	int32_t * srcBuffer = ( int32_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int32_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_srai_epi32 ( _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) ), 24 );
		_mm_store_si128 ( ( __m128i* )arr, _mm_shuffle_epi8 ( loaded, i32_to_i8_shuffle ) );
		memcpy ( destBuffer + i, arr, 4 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, int8_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int32_t, int16_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i zero_int = _mm_setzero_si128 ();

	if ( srcByteCount > destByteSize * 2 )
		return false;

	int16_t temp_arr [ 8 ];

	int32_t * srcBuffer = ( int32_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int32_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_srai_epi32 ( _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) ), 16 );
		_mm_storel_epi64 ( ( __m128i * ) temp_arr, _mm_packs_epi32 ( loaded, zero_int ) );
		memcpy ( destBuffer + i, temp_arr, 8 );
	}
	destBuffer = ( int16_t * ) dest;
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, int16_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int32_t, int24_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128i i32_to_i24_shuffle = _mm_set_epi8 ( -1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0 );

	if ( srcByteCount * 3 > destByteSize * 4 )
		return false;

	int8_t temp_arr [ 16 ];

	int32_t * srcBuffer = ( int32_t * ) src;
	int24_t * destBuffer = ( int24_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int32_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = _mm_srai_epi32 ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ), 8 );
		_mm_store_si128 ( ( __m128i * ) temp_arr, _mm_shuffle_epi8 ( loaded, i32_to_i24_shuffle ) );
		memcpy ( destBuffer + i, temp_arr, 12 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, int24_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int32_t, float> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m128  i32_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_INT );

	if ( srcByteCount > destByteSize )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	float * destBuffer = ( float * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int32_t );
	int64_t SIMDLoopCount = loopCount & SSE_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
		_mm_store_ps ( destBuffer + i, __TC_convert ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ), i32_to_f32_converter ) );
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, float> ( srcBuffer [ i ] );

	return true;
}
#	pragma endregion

static const __TC_SAMPLE_CONVERTERS __g_TC_sse_converters = {
	__TC_sample_convert_sse<float, int8_t>,
	__TC_sample_convert_sse<float, int16_t>,
	__TC_sample_convert_sse<float, int24_t>,
	__TC_sample_convert_sse<float, int32_t>,

	__TC_sample_convert_sse<int8_t, float>,
	__TC_sample_convert_sse<int8_t, int16_t>,
	__TC_sample_convert_sse<int8_t, int24_t>,
	__TC_sample_convert_sse<int8_t, int32_t>,

	__TC_sample_convert_sse<int16_t, float>,
	__TC_sample_convert_sse<int16_t, int8_t>,
	__TC_sample_convert_sse<int16_t, int24_t>,
	__TC_sample_convert_sse<int16_t, int32_t>,

	__TC_sample_convert_sse<int24_t, float>,
	__TC_sample_convert_sse<int24_t, int8_t>,
	__TC_sample_convert_sse<int24_t, int16_t>,
	__TC_sample_convert_sse<int24_t, int32_t>,

	__TC_sample_convert_sse<int32_t, float>,
	__TC_sample_convert_sse<int32_t, int8_t>,
	__TC_sample_convert_sse<int32_t, int16_t>,
	__TC_sample_convert_sse<int32_t, int24_t>,
};
#else
static const __TC_SAMPLE_CONVERTERS __g_TC_sse_converters = {};
#endif

static inline bool __TC_sse_support ()
{
#if AE_ARCH_IA32 || AE_ARCH_AMD64
	int cpuid [ 4 ];
	__cpuidex ( cpuid, 1, 0 );

	return cpuid [ 3 ] & ( 1 << 26 );
#else
	return false;
#endif
}
#pragma endregion

#pragma region	AVX Type Converter
#pragma mark	AVX Type Converter
////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//      //      //      //  //      //    //////////  //////////  //      //  //      //          //
//    //  //    //      //   //    //     //          //      //  ////    //  //      //          //
//    //  //     //    //     //  //      //          //      //  //  //  //   //    //           //
//    //////     //    //       //        //          //      //  //  //  //   //    //           //
//  //      //    //  //      //  //      //          //      //  //  //  //    //  //            //
//  //      //    //  //     //    //     //          //      //  //    ////    //  //            //
//  //      //      //      //      //    //////////  //////////  //      //      //              //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////
#if AE_ARCH_AMD64
static const __m256 __g_TC_min_value_avx = _mm256_set1_ps ( 1 );
static const __m256 __g_TC_max_value_avx = _mm256_set1_ps ( -1 );

static inline __m256 __TC_convert ( __m256i value, __m256 conv )
{
	return _mm256_min_ps ( __g_TC_min_value_avx, _mm256_max_ps ( __g_TC_max_value_avx, _mm256_mul_ps ( _mm256_cvtepi32_ps ( value ), conv ) ) );
}
static inline __m256i __TC_convert ( __m256 value, __m256 conv )
{
	return _mm256_cvtps_epi32 ( _mm256_mul_ps ( ( _mm256_min_ps ( __g_TC_min_value_avx, _mm256_max_ps ( __g_TC_max_value_avx, value ) ) ), conv ) );
}

template<typename S, typename D>
static inline bool __TC_sample_convert_avx ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static_assert ( true, "Not supported format." );
	return false;
}

#	pragma region Floating point to
#	pragma mark - Floating point to
////////////////////////////////////////////////////////////
// float to
////////////////////////////////////////////////////////////
template<> static inline bool __TC_sample_convert_avx<float, int8_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256 f32_to_i8_converter = _mm256_set1_ps ( ( float ) __TC_CHAR );
	static const __m256i i32_to_i8_shuffle = _mm256_set_epi8 ( -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 28, 24, 20, 16, 12, 8, 4, 0 );

	if ( srcByteCount > destByteSize * 4 )
		return false;

	int8_t arr [ 32 ];

	float * srcBuffer = ( float * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( float );
	int64_t SIMDLoopCount = loopCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
	{
		__m256 loadedf = _mm256_load_ps ( srcBuffer + i );
		__m256i loaded = __TC_convert ( loadedf, f32_to_i8_converter );
		loaded = _mm256_shuffle_epi8 ( loaded, i32_to_i8_shuffle );
		_mm256_store_si256 ( ( __m256i* ) arr, loaded );
		memcpy ( destBuffer + i, arr, 8 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int8_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_avx<float, int16_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256  f32_to_i16_converter = _mm256_set1_ps ( ( float ) __TC_SHORT );
	static const __m256i i32_to_i16_shuffle = _mm256_set_epi8 ( -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 29, 28, 25, 24, 21, 20, 17, 16, 13, 12, 9, 8, 5, 4, 1, 0 );

	if ( srcByteCount > destByteSize * 2 )
		return false;

	int16_t arr [ 16 ];

	float * srcBuffer = ( float * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( float );
	int64_t SIMDLoopCount = loopCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
	{
		__m256i loaded = __TC_convert ( _mm256_load_ps ( srcBuffer + i ), f32_to_i16_converter );
		_mm256_store_si256 ( ( __m256i * ) arr, _mm256_shuffle_epi8 ( loaded, i32_to_i16_shuffle ) );
		memcpy ( destBuffer + i, arr, 16 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int16_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_avx<float, int24_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256 f32_to_i24_converter = _mm256_set1_ps ( ( float ) __TC_24BIT );
	static const __m256i i32_to_i24_shuffle = _mm256_set_epi8 ( -1, -1, -1, -1, -1, -1, -1, -1, 30, 29, 28, 26, 25, 24, 22, 21, 20, 18, 17, 16, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0 );

	if ( srcByteCount * 3 > destByteSize * 4 )
		return false;

	int8_t arr [ 32 ];

	float * srcBuffer = ( float * ) src;
	int24_t * destBuffer = ( int24_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( float );
	int64_t SIMDLoopCount = loopCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
	{
		__m256i loaded = __TC_convert ( _mm256_load_ps ( srcBuffer + i ), f32_to_i24_converter );
		loaded = _mm256_shuffle_epi8 ( loaded, i32_to_i24_shuffle );
		_mm256_store_si256 ( ( __m256i * ) arr, loaded );
		memcpy ( destBuffer + i, arr, 24 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int24_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_avx<float, int32_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256 f32_to_i32_converter = _mm256_set1_ps ( ( float ) __TC_INT );

	if ( srcByteCount > destByteSize )
		return false;

	float * srcBuffer = ( float * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( float );
	int64_t SIMDLoopCount = loopCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
		_mm256_store_si256 ( ( __m256i * ) ( destBuffer + i ), __TC_convert ( _mm256_load_ps ( srcBuffer + i ), f32_to_i32_converter ) );
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int32_t> ( srcBuffer [ i ] );

	return true;
}
#	pragma endregion

#	pragma region 8-bit Integer to
#	pragma mark - 8-bit Integer to
////////////////////////////////////////////////////////////
// 8-bit to
////////////////////////////////////////////////////////////
template<> static inline bool __TC_sample_convert_avx<int8_t, int16_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256i zero_int = _mm256_setzero_si256 ();

	if ( srcByteCount * 2 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int64_t loopCount = srcByteCount & AVX512_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < loopCount; i += 16 )
	{
		__m256i loaded = _mm256_slli_epi16 ( _mm256_cvtepi8_epi16 ( _mm_load_si128 ( ( __m128i* ) ( srcBuffer + i ) ) ), 8 );
		_mm256_store_si256 ( ( __m256i* ) ( destBuffer + i ), loaded );
	}
	for ( int64_t i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = __TC_convert<int8_t, int16_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_avx<int8_t, int32_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int64_t loopCount = srcByteCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < loopCount; i += 8 )
	{
		__m256i loaded = _mm256_slli_epi32 ( _mm256_cvtepi8_epi32 ( _mm_load_si128 ( ( __m128i* ) ( srcBuffer + i ) ) ), 24 );
		_mm256_store_si256 ( ( __m256i* ) ( destBuffer + i ), loaded );
	}
	for ( int64_t i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = __TC_convert<int8_t, int32_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_avx<int8_t, float> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256  i8_to_f32_converter = _mm256_set1_ps ( ( float ) __TC_INV_CHAR );

	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	float * destBuffer = ( float * ) dest;
	int64_t loopCount = srcByteCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < loopCount; i += 8 )
	{
		__m256 loaded = __TC_convert ( _mm256_cvtepi8_epi32 ( _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) ) ), i8_to_f32_converter );
		_mm256_store_ps ( destBuffer + i, loaded );
	}
	for ( int64_t i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = __TC_convert<int8_t, float> ( srcBuffer [ i ] );

	return true;
}

#	pragma endregion

#	pragma region 16-bit Integer to
#	pragma mark - 16-bit Integer to
////////////////////////////////////////////////////////////
// 16-bit to
////////////////////////////////////////////////////////////
template<> static inline bool __TC_sample_convert_avx<int16_t, int8_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256i i16_to_i8_shuffle = _mm256_set_epi8 ( -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0 );
	
	if ( srcByteCount > destByteSize * 2 )
		return false;

	int8_t arr [ 32 ];

	int16_t * srcBuffer = ( int16_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int16_t );
	int64_t SIMDLoopCount = loopCount & AVX512_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 16 )
	{
		__m256i loaded = _mm256_srai_epi16 ( _mm256_load_si256 ( ( __m256i * ) ( srcBuffer + i ) ), 8 );
		_mm256_store_si256 ( ( __m256i* ) arr, _mm256_shuffle_epi8 ( loaded, i16_to_i8_shuffle ) );
		memcpy ( destBuffer + i, arr, 16 );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, int8_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_avx<int16_t, int32_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int16_t );
	int64_t SIMDLoopCount = loopCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
	{
		__m256i loaded = _mm256_slli_epi32 ( _mm256_cvtepi16_epi32 ( _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) ) ), 16 );
		_mm256_store_si256 ( ( __m256i* ) ( destBuffer + i ), loaded );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, int32_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_avx<int16_t, float> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256  i16_to_f32_converter = _mm256_set1_ps ( ( float ) __TC_INV_SHORT );

	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	float * destBuffer = ( float * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int16_t );
	int64_t SIMDLoopCount = loopCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
		_mm256_store_ps ( destBuffer + i, __TC_convert ( _mm256_cvtepi16_epi32 ( _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) ) ), i16_to_f32_converter ) );
	destBuffer = ( float * ) dest;
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, float> ( srcBuffer [ i ] );

	return true;
}
#	pragma endregion

#	pragma region 24-bit Integer to
#	pragma mark - 24-bit Integer to
////////////////////////////////////////////////////////////
// 24-bit to
////////////////////////////////////////////////////////////

#	pragma endregion

#	pragma region 32-bit Integer to
#	pragma mark - 32-bit Integer to
////////////////////////////////////////////////////////////
// 32-bit to
////////////////////////////////////////////////////////////
template<> static inline bool __TC_sample_convert_avx<int32_t, int24_t> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256i i32_to_i24_shuffle = _mm256_set_epi8 ( -1, -1, -1, -1, -1, -1, -1, -1, 30, 29, 28, 26, 25, 24, 22, 21, 20, 18, 17, 16, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0 );

	if ( srcByteCount * 3 > destByteSize * 4 )
		return false;

	int8_t arr [ 32 ];

	int32_t * srcBuffer = ( int32_t * ) src;
	int24_t * destBuffer = ( int24_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int32_t );
	int64_t SIMDLoopCount = loopCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
	{
		__m256i loaded = _mm256_srai_epi32 ( _mm256_load_si256 ( ( const __m256i * ) &srcBuffer [ i ] ), 8 );
 		_mm256_store_si256 ( ( __m256i* ) arr, _mm256_shuffle_epi8 ( loaded, i32_to_i24_shuffle ) );
		memcpy ( destBuffer + i, arr, 8 * sizeof ( int24_t ) );
	}
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, int24_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_avx<int32_t, float> ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize ) noexcept
{
	static const __m256  i32_to_f32_converter = _mm256_set1_ps ( ( float ) __TC_INV_INT );

	if ( srcByteCount > destByteSize )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	float * destBuffer = ( float * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int32_t );
	int64_t SIMDLoopCount = loopCount & AVX_CONVERT_COUNT_UNIT;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
		_mm256_store_ps ( destBuffer + i, __TC_convert ( _mm256_load_si256 ( ( const __m256i * ) &srcBuffer [ i ] ), i32_to_f32_converter ) );
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, float> ( srcBuffer [ i ] );

	return true;
}
#	pragma endregion

static const __TC_SAMPLE_CONVERTERS __g_TC_avx_converters = {
	__TC_sample_convert_avx<float, int8_t>,
	__TC_sample_convert_avx<float, int16_t>,
	__TC_sample_convert_avx<float, int24_t>,
	__TC_sample_convert_avx<float, int32_t>,

	__TC_sample_convert_avx<int8_t, float>,
	__TC_sample_convert_avx<int8_t, int16_t>,
	__TC_sample_convert_sse<int8_t, int24_t>,
	__TC_sample_convert_avx<int8_t, int32_t>,

	__TC_sample_convert_avx<int16_t, float>,
	__TC_sample_convert_avx<int16_t, int8_t>,
	__TC_sample_convert_sse<int16_t, int24_t>,
	__TC_sample_convert_avx<int16_t, int32_t>,

	__TC_sample_convert_sse<int24_t, float>,
	__TC_sample_convert_sse<int24_t, int8_t>,
	__TC_sample_convert_sse<int24_t, int16_t>,
	__TC_sample_convert_sse<int24_t, int32_t>,

	__TC_sample_convert_avx<int32_t, float>,
	__TC_sample_convert_sse<int32_t, int8_t>,
	__TC_sample_convert_sse<int32_t, int16_t>,
	__TC_sample_convert_avx<int32_t, int24_t>,
};
#else
static const __TC_SAMPLE_CONVERTERS __g_TC_avx_converters = {};
#endif

static inline bool __TC_avx_support ()
{
#if AE_ARCH_AMD64
	int cpuid [ 4 ];
	__cpuidex ( cpuid, 1, 0 );

	return cpuid [ 2 ] & ( 1 << 28 );
#else
	return false;
#endif
}
#pragma endregion

#pragma region	NEON Type Converter
#pragma mark	NEON Type Converter
////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                //
//  //      //  //////////  //////////  //      //    //////////  //////////  //      //  //      //
//  ////    //  //          //      //  ////    //    //          //      //  ////    //  //      //
//  //  //  //  //          //      //  //  //  //    //          //      //  //  //  //   //    ///
//  //  //  //  //////////  //      //  //  //  //    //          //      //  //  //  //   //    ///
//  //  //  //  //          //      //  //  //  //    //          //      //  //  //  //    //  ////
//  //    ////  //          //      //  //    ////    //          //      //  //    ////    //  ////
//  //      //  //////////  //////////  //      //    //////////  //////////  //      //      //  //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////
#if AE_ARCH_ARM || AE_ARCH_ARM64

static const __TC_SAMPLE_CONVERTERS __g_TC_neon_converters = {

};
#else
static const __TC_SAMPLE_CONVERTERS __g_TC_neon_converters = {};
#endif

static inline bool __TC_neon_support ()
{
#if AE_ARCH_ARM || AE_ARCH_ARM64
#	if AE_PLATFORM_ANDROID
	return ( android_getCpuFamily () == ANDROID_CPU_FAMILY_ARM
		&& ( android_getCpuFeature () & ANDROID_CPU_ARM_FEATURE_NEON ) != 0 );
#	elif AE_PLATFORM_IOS
	return true;
#	else
	return false;
#	endif
#else
	return false;
#endif
}
#pragma endregion

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

enum TypeConverterSupport
{
	TCS_PLAIN = 0,
	TCS_SSE = 1,
	TCS_AVX = 2,
	TCS_NEON = 3,
};

static inline bool __TC_is_support_converter ( TypeConverterSupport tcs )
{
	switch ( tcs )
	{
		case TCS_PLAIN: return __TC_plain_support ();
		case TCS_SSE: return __TC_sse_support ();
		case TCS_AVX: return __TC_avx_support ();
		//case TCS_NEON: return __TC_neon_support ();
		default: return false;
	}
}

static inline const __TC_SAMPLE_CONVERTERS & __TC_get_optimal_converter ()
{
	struct { TypeConverterSupport tcs; const __TC_SAMPLE_CONVERTERS & conv; } convs [] = {
		{ TCS_NEON,  __g_TC_neon_converters },
		{ TCS_AVX,   __g_TC_avx_converters },
		{ TCS_SSE,   __g_TC_sse_converters },
	};
	for ( auto conv : convs )
	{
		if ( __TC_is_support_converter ( conv.tcs ) )
			return conv.conv;
	}

	return __g_TC_plain_converters;
}

#endif