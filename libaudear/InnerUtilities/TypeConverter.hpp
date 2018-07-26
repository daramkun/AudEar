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
	int8_t value [ 3 ];

	int24_t () = default;
	inline int24_t ( int32_t v )
	{
		/*value [ 2 ] = ( int8_t ) ( ( v >> 16 ) & 0xff );
		value [ 1 ] = ( int8_t ) ( ( v >> 8 ) & 0xff );
		value [ 0 ] = ( int8_t ) ( v & 0xff );*/
		memcpy ( value, &v, 3 );
	}

	inline operator int32_t () const
	{
		return ( ( value [ 2 ] & 0x80 ) ? ( 0xff << 24 ) : 0 )
			| ( ( ( ( int32_t ) value [ 2 ] ) << 16 )/* 0x00ff0000*/ )
			| ( ( ( ( int32_t ) value [ 1 ] ) <<  8 )/* & 0x0000ff00*/ )
			| ( ( ( ( int32_t ) value [ 0 ] ) <<  0 )/* & 0x000000ff*/ );
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
template<> static inline int16_t __TC_convert ( int8_t value ) { return __TC_convert<float, int16_t> ( __TC_convert<int8_t, float> ( value ) ); }
template<> static inline int24_t __TC_convert ( int8_t value ) { return __TC_convert<float, int24_t> ( __TC_convert<int24_t, float> ( value ) ); }
template<> static inline int32_t __TC_convert ( int8_t value ) { return __TC_convert<float, int32_t> ( __TC_convert<int32_t, float> ( value ) ); }
template<> static inline int8_t __TC_convert ( int16_t value ) { return __TC_convert<float, int8_t> ( __TC_convert<int16_t, float> ( value ) ); }
template<> static inline int24_t __TC_convert ( int16_t value ) { return __TC_convert<float, int24_t> ( __TC_convert<int16_t, float> ( value ) ); }
template<> static inline int32_t __TC_convert ( int16_t value ) { return __TC_convert<float, int32_t> ( __TC_convert<int16_t, float> ( value ) ); }
template<> static inline int8_t __TC_convert ( int24_t value ) { return __TC_convert<float, int8_t> ( __TC_convert<int24_t, float> ( value ) ); }
template<> static inline int16_t __TC_convert ( int24_t value ) { return __TC_convert<float, int16_t> ( __TC_convert<int24_t, float> ( value ) ); }
template<> static inline int32_t __TC_convert ( int24_t value ) { return __TC_convert<float, int32_t> ( __TC_convert<int24_t, float> ( value ) ); }
template<> static inline int8_t __TC_convert ( int32_t value ) { return __TC_convert<float, int8_t> ( __TC_convert<int32_t, float> ( value ) ); }
template<> static inline int16_t __TC_convert ( int32_t value ) { return __TC_convert<float, int16_t> ( __TC_convert<int32_t, float> ( value ) ); }
template<> static inline int24_t __TC_convert ( int32_t value ) { return __TC_convert<float, int24_t> ( __TC_convert<int32_t, float> ( value ) ); }

typedef bool ( *__TypeConverterFunction ) ( void * src, void * dest, int srcByteCount, int destByteSize );
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
			else if ( std::is_same<D, int16_t>::value ) return i16_to_i8;
		}
		else if ( std::is_same<S, int24_t>::value )
		{
			if ( std::is_same<D, float>::value ) return i24_to_f32;
			else if ( std::is_same<D, int24_t>::value ) return i24_to_i16;
			else if ( std::is_same<D, int32_t>::value ) return i24_to_i32;
			else if ( std::is_same<D, int16_t>::value ) return i24_to_i8;
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
			else if ( std::is_same<D, int24_t>::value ) return i8_to_i24;
			else if ( std::is_same<D, int32_t>::value ) return i8_to_i32;
			else if ( std::is_same<D, int16_t>::value ) return i8_to_i16;
		}
		static_assert ( true, "Not supported format." );
		return nullptr;
	}
};

template <typename S, typename D>
static inline bool __TC_sample_convert ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * sizeof ( D ) > destByteSize * sizeof ( S ) )
		return false;

	S * srcBuffer = ( S* ) src;
	D * destBuffer = ( D* ) dest;
	int loopCount = srcByteCount / sizeof ( S );
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
static const __m128i __g_TC_zero_int = _mm_setzero_si128 ();
static const __m128  __g_TC_zero_float = _mm_setzero_ps ();

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
static inline bool __TC_sample_convert_sse ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static_assert ( true, "Not supported format." );
	return false;
}

template<> static inline bool __TC_sample_convert_sse<float, int8_t> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128 f32_to_i8_converter = _mm_set1_ps ( ( float ) __TC_CHAR );
	
	if ( srcByteCount > destByteSize * 4 )
		return false;

	float * srcBuffer = ( float * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = __TC_convert ( _mm_load_ps ( srcBuffer + i ), f32_to_i8_converter );
		//int8_t arr [ 4 ];
		//loaded = _mm_packs_epi32 ( loaded, __g_TC_zero_int );
		//loaded = _mm_packus_epi16 ( loaded, __g_TC_zero_int );
		//*( ( int32_t * ) arr ) = _mm_cvtsi128_si32 ( loaded );
		//memcpy ( destBuffer + 4, arr, 4 );
		int32_t arr [ 4 ];
		_mm_store_si128 ( ( __m128i* )arr, loaded );
		destBuffer [ i + 0 ] = ( int8_t ) arr [ 0 ];
		destBuffer [ i + 1 ] = ( int8_t ) arr [ 1 ];
		destBuffer [ i + 2 ] = ( int8_t ) arr [ 2 ];
		destBuffer [ i + 3 ] = ( int8_t ) arr [ 3 ];
	}
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int8_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<float, int16_t> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128 f32_to_i16_converter = _mm_set1_ps ( ( float ) __TC_SHORT );

	if ( srcByteCount > destByteSize * 2 )
		return false;

	float * srcBuffer = ( float * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = __TC_convert ( _mm_load_ps ( srcBuffer + i ), f32_to_i16_converter );
		int16_t arr [ 8 ];
		_mm_storel_epi64 ( ( __m128i * ) &arr, _mm_packs_epi32 ( loaded, __g_TC_zero_int ) );
		memcpy ( destBuffer + i, arr, 8 );
	}
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int16_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<float, int24_t> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128 f32_to_i24_converter = _mm_set1_ps ( ( float ) __TC_24BIT );

	if ( srcByteCount * 3 > destByteSize * 4 )
		return false;

	float * srcBuffer = ( float * ) src;
	int24_t * destBuffer = ( int24_t * ) dest;
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = __TC_convert ( _mm_load_ps ( srcBuffer + i ), f32_to_i24_converter );
		int32_t arr [ 4 ];
		_mm_store_si128 ( ( __m128i * ) &arr, loaded );
		destBuffer [ i + 0 ] = ( int24_t ) arr [ 0 ];
		destBuffer [ i + 1 ] = ( int24_t ) arr [ 1 ];
		destBuffer [ i + 2 ] = ( int24_t ) arr [ 2 ];
		destBuffer [ i + 3 ] = ( int24_t ) arr [ 3 ];
	}
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int24_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<float, int32_t> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128 f32_to_i32_converter = _mm_set1_ps ( ( float ) __TC_INV_INT );

	if ( srcByteCount > destByteSize )
		return false;

	float * srcBuffer = ( float * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
		_mm_store_si128 ( ( __m128i * ) ( destBuffer + i ), __TC_convert ( _mm_load_ps ( srcBuffer + i ), f32_to_i32_converter ) );
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<float, int32_t> ( srcBuffer [ i ] );

	return true;
}
/*static inline bool __TC_sample_convert_sse_i8_i16 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 2 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int loopCount = srcByteCount / 4 * 4;
	for ( int i = 0; i < loopCount; i += 4 )
	{
		__m128i loaded = _mm_cvtepi8_epi32 ( _mm_loadl_epi64 ( ( __m128i* ) ( srcBuffer + i ) ) );
		__m128 conv = __TC_convert ( loaded, __g_TC_int8_to_float_converter );
		loaded = __TC_convert ( conv, __g_TC_float_to_int16_converter );
		_mm_storel_epi64 ( ( __m128i* ) ( destBuffer + 4 ), _mm_cvtepi32_epi16 ( loaded ) );
	}
	for ( int i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = __TC_convert<int8_t, int16_t> ( srcBuffer [ i ] );

	return true;
}
static inline bool __TC_sample_convert_sse_i8_i32 ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int loopCount = srcByteCount / 4 * 4;
	for ( int i = 0; i < loopCount; i += 4 )
	{
		__m128i loaded = _mm_cvtepi8_epi32 ( _mm_loadl_epi64 ( ( __m128i* ) ( srcBuffer + i ) ) );
		__m128 conv = __TC_convert ( loaded, __g_TC_int8_to_float_converter );
		loaded = __TC_convert ( conv, __g_TC_float_to_int32_converter );
		_mm_store_si128 ( ( __m128i* ) ( destBuffer + 4 ), loaded );
	}
	for ( int i = loopCount; i < srcByteCount; ++i )
		destBuffer [ i ] = ( int32_t ) ( ( ( int32_t ) srcBuffer [ i ] ) * 16777216 );

	return true;
}
__TYPECONVETERFUNC ( int8, float, sse )
{
	if ( srcByteCount * 4 > destByteSize )
		return false;

	int8_t * srcBuffer = ( int8_t * ) src;
	float * destBuffer = ( float * ) dest;
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

	return true;
}
__TYPECONVETERFUNC ( int16, int8, sse )
{
	if ( srcByteCount > destByteSize * 2 )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 2;
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

	return true;
}*/
template<> static inline bool __TC_sample_convert_sse<int16_t, int32_t> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128  i16_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_SHORT );
	static const __m128  f32_to_i32_converter = _mm_set1_ps ( ( float ) __TC_INT );

	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	int32_t * destBuffer = ( int32_t * ) dest;
	int loopCount = srcByteCount / 2;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = __TC_convert ( __TC_convert ( _mm_cvtepi16_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ), i16_to_f32_converter ), f32_to_i32_converter );
		_mm_store_si128 ( ( __m128i* ) ( destBuffer + i ), loaded );
	}
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, int32_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int16_t, float> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128  i16_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_SHORT );

	if ( srcByteCount * 2 > destByteSize )
		return false;

	int16_t * srcBuffer = ( int16_t * ) src;
	float * destBuffer = ( float * ) dest;
	int loopCount = srcByteCount / 2;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
		_mm_store_ps ( destBuffer + i, __TC_convert ( _mm_cvtepi16_epi32 ( _mm_loadl_epi64 ( ( __m128i * ) ( srcBuffer + i ) ) ), i16_to_f32_converter ) );
	destBuffer = ( float * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int16_t, float> ( srcBuffer [ i ] );

	return true;
}

template<> static inline bool __TC_sample_convert_sse<int32_t, int8_t> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128  i32_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_INT );
	static const __m128  f32_to_i8_converter = _mm_set1_ps ( ( float ) __TC_CHAR );
	
	if ( srcByteCount > destByteSize * 4 )
		return false;

	int32_t arr [ 4 ];

	int32_t * srcBuffer = ( int32_t * ) src;
	int8_t * destBuffer = ( int8_t * ) dest;
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = __TC_convert ( __TC_convert ( _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) ), i32_to_f32_converter ), f32_to_i8_converter );
		//int8_t arr [ 4 ];
		//loaded = _mm_packs_epi32 ( loaded, loaded );
		//loaded = _mm_packus_epi16 ( loaded, loaded );
		//*( ( int32_t* ) arr ) = _mm_cvtsi128_si32 ( loaded );
		//memcpy ( destBuffer + 4, arr, 4 );
		_mm_store_si128 ( ( __m128i* )arr, loaded );
		destBuffer [ i + 0 ] = ( int8_t ) arr [ 0 ];
		destBuffer [ i + 1 ] = ( int8_t ) arr [ 1 ];
		destBuffer [ i + 2 ] = ( int8_t ) arr [ 2 ];
		destBuffer [ i + 3 ] = ( int8_t ) arr [ 3 ];
	}
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, int8_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int32_t, int16_t> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128  i32_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_INT );
	static const __m128  f32_to_i16_converter = _mm_set1_ps ( ( float ) __TC_SHORT );
	static const __m128i zero_int = _mm_setzero_si128 ();

	if ( srcByteCount > destByteSize * 2 )
		return false;

	int16_t temp_arr [ 8 ];

	int32_t * srcBuffer = ( int32_t * ) src;
	int16_t * destBuffer = ( int16_t * ) dest;
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = __TC_convert ( __TC_convert ( _mm_load_si128 ( ( __m128i * ) ( srcBuffer + i ) ), i32_to_f32_converter ), f32_to_i16_converter );
		_mm_storel_epi64 ( ( __m128i * ) temp_arr, _mm_packs_epi32 ( loaded, zero_int ) );
		memcpy ( destBuffer + i, temp_arr, 8 );
	}
	destBuffer = ( int16_t * ) dest;
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, int16_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int32_t, int24_t> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128  i32_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_INT );
	static const __m128  f32_to_i24_converter = _mm_set1_ps ( ( float ) __TC_24BIT );

	if ( srcByteCount * 3 > destByteSize * 4 )
		return false;

	int32_t temp_arr [ 4 ];

	int32_t * srcBuffer = ( int32_t * ) src;
	int24_t * destBuffer = ( int24_t * ) dest;
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
	{
		__m128i loaded = __TC_convert ( __TC_convert ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ), i32_to_f32_converter ), f32_to_i24_converter );
		_mm_store_si128 ( ( __m128i * ) temp_arr, loaded );
		destBuffer [ i + 0 ] = ( int24_t ) temp_arr [ 0 ];
		destBuffer [ i + 1 ] = ( int24_t ) temp_arr [ 1 ];
		destBuffer [ i + 2 ] = ( int24_t ) temp_arr [ 2 ];
		destBuffer [ i + 3 ] = ( int24_t ) temp_arr [ 3 ];
	}
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, int24_t> ( srcBuffer [ i ] );

	return true;
}
template<> static inline bool __TC_sample_convert_sse<int32_t, float> ( void * src, void * dest, int srcByteCount, int destByteSize ) noexcept
{
	static const __m128  i32_to_f32_converter = _mm_set1_ps ( ( float ) __TC_INV_INT );

	if ( srcByteCount > destByteSize )
		return false;

	int32_t * srcBuffer = ( int32_t * ) src;
	float * destBuffer = ( float * ) dest;
	int loopCount = srcByteCount / 4;
	int SIMDLoopCount = loopCount / 4 * 4;
	for ( int i = 0; i < SIMDLoopCount; i += 4 )
		_mm_store_ps ( destBuffer + i, __TC_convert ( _mm_load_si128 ( ( const __m128i * ) &srcBuffer [ i ] ), i32_to_f32_converter ) );
	for ( int i = SIMDLoopCount; i < loopCount; ++i )
		destBuffer [ i ] = __TC_convert<int32_t, float> ( srcBuffer [ i ] );

	return true;
}

static const __TC_SAMPLE_CONVERTERS __g_TC_sse_converters = {
	__TC_sample_convert_sse<float, int8_t>,
	__TC_sample_convert_sse<float, int16_t>,
	__TC_sample_convert_sse<float, int24_t>,
	__TC_sample_convert_sse<float, int32_t>,

	__TC_sample_convert<int8_t, float>,
	__TC_sample_convert<int8_t, int16_t>,
	__TC_sample_convert<int8_t, int24_t>,
	__TC_sample_convert<int8_t, int32_t>,

	__TC_sample_convert_sse<int16_t, float>,
	__TC_sample_convert<int16_t, int8_t>,
	__TC_sample_convert<int16_t, int24_t>,
	__TC_sample_convert_sse<int16_t, int32_t>,

	__TC_sample_convert<int24_t, float>,
	__TC_sample_convert<int24_t, int8_t>,
	__TC_sample_convert<int24_t, int16_t>,
	__TC_sample_convert<int24_t, int32_t>,

	__TC_sample_convert_sse<int32_t, float>,
	__TC_sample_convert_sse<int32_t, int8_t>,
	__TC_sample_convert_sse<int32_t, int16_t>,
	__TC_sample_convert_sse<int32_t, int24_t>,
};
#endif

#endif