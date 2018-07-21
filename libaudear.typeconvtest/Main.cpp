//#define USE_SIMD 0
#define USE_SIMD 1

#include "../libaudear/audear.preprocessors.h"
#include "../libaudear/InnerUtilities/TypeConverter.hpp"
#include "../libaudear/InnerUtilities/HighResolutionTimer.hpp"

#include <memory>

#define LOOP_COUNT											5

#define SAMPLE_CHANNELS										2
#define SAMPLE_SIZE											48000
#define SAMPLE_SOURCE_TYPE									float
#define SAMPLE_DESTINATION_TYPE								int16_t

int main ( void )
{
	printf ( "Process Architecture: %s(%s), SIMD Usage: %c\n",
#if AE_ARCH_IA32
		"x86"
#elif AE_ARCH_AMD64
		"x64"
#endif
		,
#if _DEBUG
		"Debug"
#else
		"Release"
#endif
		,
#if USE_SIMD
		'Y'
#else
		'N'
#endif
	);

	printf ( "Sample information: %dch, %dHz\n", SAMPLE_CHANNELS, SAMPLE_SIZE );
	printf ( "Bits per Sample: source - %d, destination - %d\n", ( int ) sizeof ( SAMPLE_SOURCE_TYPE ) * 8, ( int ) sizeof ( SAMPLE_DESTINATION_TYPE ) * 8 );

	int64_t freq = __HRT_GetFrequency (), start, end;

	using namespace std;

	shared_ptr<SAMPLE_SOURCE_TYPE []> source ( new SAMPLE_SOURCE_TYPE [ SAMPLE_SIZE * SAMPLE_CHANNELS ] );
	shared_ptr<SAMPLE_DESTINATION_TYPE []> destination ( new SAMPLE_DESTINATION_TYPE [ SAMPLE_SIZE * SAMPLE_CHANNELS ] );

	__TypeConverterFunction converter;
	if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( int8_t ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( int16_t ) )
		converter = __TC_int8_to_int16;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( int8_t ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( int32_t ) )
		converter = __TC_int8_to_int32;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( int8_t ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( float ) )
		converter = __TC_int8_to_float;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( int16_t ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( int8_t ) )
		converter = __TC_int16_to_int8;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( int16_t ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( int32_t ) )
		converter = __TC_int16_to_int32;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( int16_t ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( float ) )
		converter = __TC_int16_to_float;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( int32_t ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( int8_t ) )
		converter = __TC_int32_to_int8;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( int32_t ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( int16_t ) )
		converter = __TC_int32_to_int16;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( int32_t ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( float ) )
		converter = __TC_int32_to_float;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( float ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( int8_t ) )
		converter = __TC_float_to_int8;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( float ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( int16_t ) )
		converter = __TC_float_to_int16;
	else if ( typeid ( SAMPLE_SOURCE_TYPE ) == typeid ( float ) && typeid ( SAMPLE_DESTINATION_TYPE ) == typeid ( int32_t ) )
		converter = __TC_float_to_int32;
	else
		return -1;

	for ( int i = 0; i < LOOP_COUNT; ++i )
	{
		for ( int i = 0; i < SAMPLE_SIZE * SAMPLE_CHANNELS; ++i )
			source [ i ] = ( SAMPLE_SOURCE_TYPE ) ( destination [ i ] * rand () );

		start = __HRT_GetCounter ();
		converter ( &source [ 0 ], &destination [ 0 ],
			SAMPLE_SIZE * SAMPLE_CHANNELS * sizeof ( SAMPLE_SOURCE_TYPE ),
			SAMPLE_SIZE * SAMPLE_CHANNELS * sizeof ( SAMPLE_DESTINATION_TYPE ) );
		end = __HRT_GetCounter ();

		printf ( "%d. Converted latency: %lfs\n", i, ( end - start ) / ( double ) freq );
	}

	return 0;
}