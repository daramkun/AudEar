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
#include <cstring>
#include <cmath>
#include <type_traits>

#if ( AE_ARCH_IA32 || AE_ARCH_AMD64 )
//  SSE 2
#	include <mmintrin.h>
#	include <emmintrin.h>
//  SSE 3
#	include <immintrin.h>
#	include <pmmintrin.h>
//  SSE 4
#	include <smmintrin.h>
#elif ( AE_ARCH_ARM || AE_ARCH_ARM64 )

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

typedef bool ( *__TypeConverterFunction ) ( const void * src, void * dest, int64_t srcByteCount, int64_t destByteSize );
struct __TC_SAMPLE_CONVERTERS
{
	__TypeConverterFunction f32_to_i8;
	__TypeConverterFunction f32_to_i16;
	__TypeConverterFunction f32_to_i24;
	__TypeConverterFunction f32_to_i32;

	__TypeConverterFunction i8_to_f32;
	__TypeConverterFunction i8_to_i16;
	__TypeConverterFunction i8_to_i24;
	__TypeConverterFunction i8_to_i32;

	__TypeConverterFunction i16_to_f32;
	__TypeConverterFunction i16_to_i8;
	__TypeConverterFunction i16_to_i24;
	__TypeConverterFunction i16_to_i32;

	__TypeConverterFunction i24_to_f32;
	__TypeConverterFunction i24_to_i8;
	__TypeConverterFunction i24_to_i16;
	__TypeConverterFunction i24_to_i32;

	__TypeConverterFunction i32_to_f32;
	__TypeConverterFunction i32_to_i8;
	__TypeConverterFunction i32_to_i16;
	__TypeConverterFunction i32_to_i24;

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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	static const __m128 f32_to_i32_converter = _mm_set1_ps ( ( float ) __TC_INV_INT );

	if ( srcByteCount > destByteSize )
		return false;

	float * srcBuffer = ( float * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( float );
	int64_t SIMDLoopCount = loopCount / 4 * 4;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
		_mm_store_si128 ( ( __m128i * ) ( destBuffer + i ), __TC_convert ( _mm_load_ps ( srcBuffer + i ), f32_to_i32_converter ) );
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int32_t> ( srcBuffer [ i ] );

	return true;
}

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
	int64_t loopCount = srcByteCount / 8 * 8;
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
	int64_t loopCount = srcByteCount / 4 * 4;
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
	int64_t loopCount = srcByteCount / 4 * 4;
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
	int64_t loopCount = srcByteCount / 4 * 4;
	for ( int64_t i = 0; i < loopCount; i += 4 )
	{
		__m128 loaded = __TC_convert ( _mm_cvtepi8_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ), i8_to_f32_converter );
		_mm_store_ps ( destBuffer + i, loaded );
	}
	for ( int64_t i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = __TC_convert<int8_t, float> ( srcBuffer [ i ] );

	return true;
}

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
	int64_t SIMDLoopCount = loopCount / 8 * 8;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 8 )
	{
		__m128i loaded = _mm_srai_epi16 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ), 8 );
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	static const __m128  i16_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_SHORT );
	static const __m128  f32_to_i32_converter = _mm_set1_ps ( ( float ) __TC_INT );

	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int64_t loopCount = srcByteCount / sizeof ( int16_t );
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
		_mm_store_ps ( destBuffer + i, __TC_convert ( _mm_cvtepi16_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ), i16_to_f32_converter ) );
	destBuffer = ( float * ) dest;
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, float> ( srcBuffer [ i ] );

	return true;
}

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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
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
	int64_t SIMDLoopCount = loopCount / 4 * 4;
	for ( int64_t i = 0; i < SIMDLoopCount; i += 4 )
		_mm_store_ps ( destBuffer + i, __TC_convert ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ), i32_to_f32_converter ) );
	for ( int64_t i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, float> ( srcBuffer [ i ] );

	return true;
}

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
#endif

#endif