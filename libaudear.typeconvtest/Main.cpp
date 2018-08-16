#include "../libaudear/audear.preprocessors.h"
#include "../libaudear/audear.struct.h"
#include "../libaudear/InnerUtilities/TypeConverter.hpp"
#include "../libaudear/InnerUtilities/HighResolutionTimer.hpp"

#include <ctime>
#include <memory>

#if AE_ARCH_IA32
#	define ARCHITECTURE										"x86"
#elif AE_ARCH_AMD64
#	define ARCHITECTURE										"x64"
#else
#	define ARCHITECTURE										"Unknown"
#endif
#if _DEBUG
#	define TARGET											"Debug"
#else
#	define TARGET											"Release"
#endif

#define LOOP_COUNT											5

#define SAMPLE_CHANNELS										2
#define SAMPLE_SIZE											48000
#define SAMPLE_SOURCE_TYPE									int32_t
#define SAMPLE_DESTINATION_TYPE								int8_t

void __check_valid ( const __TC_SAMPLE_CONVERTERS & converters, const SAMPLE_SOURCE_TYPE * source1, SAMPLE_SOURCE_TYPE * source2, SAMPLE_DESTINATION_TYPE * destination )
{
	using namespace std;

	printf ( "Valid: " );

	if ( !converters.get_converter<SAMPLE_SOURCE_TYPE, SAMPLE_DESTINATION_TYPE> () (
		source1, destination,
		SAMPLE_SIZE * SAMPLE_CHANNELS * sizeof ( SAMPLE_SOURCE_TYPE ),
		SAMPLE_SIZE * SAMPLE_CHANNELS * sizeof ( SAMPLE_DESTINATION_TYPE ) ) )
	{
		printf ( "F(Failed call S->D)\n" );
		return;
	}

	if ( !converters.get_converter<SAMPLE_DESTINATION_TYPE, SAMPLE_SOURCE_TYPE> () (
		destination, source2,
		SAMPLE_SIZE * SAMPLE_CHANNELS * sizeof ( SAMPLE_DESTINATION_TYPE ),
		SAMPLE_SIZE * SAMPLE_CHANNELS * sizeof ( SAMPLE_SOURCE_TYPE ) ) )
	{
		printf ( "F(Failed call D->S)\n" );
		return;
	}

	bool printFail = false;
	int failedCount = 0;
	for ( int i = 0; i < SAMPLE_SIZE * SAMPLE_CHANNELS; ++i )
	{
		if ( source1 [ i ] != source2 [ i ] )
		{
			bool diff = false;
			if ( std::is_same<SAMPLE_SOURCE_TYPE, float>::value )
			{
				if ( ( source1 [ i ] - source2 [ i ] ) > FLT_EPSILON )
					diff = true;
			}
			else
			{
				int sm = ( int ) pow ( 2, sizeof ( SAMPLE_SOURCE_TYPE ) * 8 - 1 ) - 1;
				int dm = ( int ) pow ( 2, sizeof ( SAMPLE_DESTINATION_TYPE ) * 8 - 1 ) - 1;
				if ( abs ( source1 [ i ] - source2 [ i ] ) > ( max ( sm, dm ) / min ( sm, dm ) ) )
					diff = true;
			}

			if ( diff && !printFail )
			{
				printf ( "F(%f to %f, Diff: %f, ",
					( float ) source1 [ i ], ( float ) source2 [ i ],
					( float ) ( source1 [ i ] - source2 [ i ] ) );
				printFail = true;
			}

			if ( diff )
				++failedCount;
		}
	}

	if ( failedCount > 0 )
		printf ( "Failed ratio: %f%%(%d times))\n", failedCount / ( float ) ( SAMPLE_SIZE * SAMPLE_CHANNELS ) * 100, failedCount );
	else
		printf ( "T\n" );
}

void __measure ( const __TC_SAMPLE_CONVERTERS & converters )
{
	static int64_t freq = __HRT_GetFrequency ();
	int64_t start, end;

	using namespace std;

	AEAUDIOBUFFER<SAMPLE_SOURCE_TYPE> source ( SAMPLE_SIZE * SAMPLE_CHANNELS );
	AEAUDIOBUFFER<SAMPLE_DESTINATION_TYPE> destination ( SAMPLE_SIZE * SAMPLE_CHANNELS );
	
	__TypeConverterFunction converter = converters.get_converter<SAMPLE_SOURCE_TYPE, SAMPLE_DESTINATION_TYPE> ();
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
}

int main ( void )
{
	printf ( "Process Architecture: %s(%s)\n", ARCHITECTURE, TARGET );

	printf ( "Sample information: %dch, %dHz\n", SAMPLE_CHANNELS, SAMPLE_SIZE );
	printf ( "Bits per Sample: source - %d, destination - %d\n", ( int ) sizeof ( SAMPLE_SOURCE_TYPE ) * 8, ( int ) sizeof ( SAMPLE_DESTINATION_TYPE ) * 8 );

	AEAUDIOBUFFER<SAMPLE_SOURCE_TYPE> source1 ( SAMPLE_SIZE * SAMPLE_CHANNELS );
	AEAUDIOBUFFER<SAMPLE_DESTINATION_TYPE> destination ( SAMPLE_SIZE * SAMPLE_CHANNELS );
	AEAUDIOBUFFER<SAMPLE_SOURCE_TYPE> source2 ( SAMPLE_SIZE * SAMPLE_CHANNELS );

	srand ( ( uint32_t ) time ( nullptr ) );
	if ( std::is_same<SAMPLE_SOURCE_TYPE, float>::value )
	{
		if ( !std::is_same<SAMPLE_DESTINATION_TYPE, float>::value )
		{
			int value = ( int ) pow ( 2, sizeof ( SAMPLE_DESTINATION_TYPE ) * 8 - 1 ) - 1;
			for ( int i = 0; i < SAMPLE_SIZE * SAMPLE_CHANNELS; ++i )
				source1 [ i ] = ( rand () % value ) / value;
		}
		else
		{
			for ( int i = 0; i < SAMPLE_SIZE * SAMPLE_CHANNELS; ++i )
				source1 [ i ] = ( SAMPLE_SOURCE_TYPE ) ( rand () / ( float ) INT_MAX );
		}
	}
	else
	{
		int value = ( int ) pow ( 2, sizeof ( SAMPLE_DESTINATION_TYPE ) * 8 - 1 ) - 1;
		for ( int i = 0; i < SAMPLE_SIZE * SAMPLE_CHANNELS; ++i )
			source1 [ i ] = rand () % value;
	}

	printf ( "\n== Plain C++ Code ==\n" );
	__check_valid ( __g_TC_plain_converters, source1, source2, destination );
	__measure ( __g_TC_plain_converters );

	printf ( "\n== SIMD Code (SSE) ==\n" );
	__check_valid ( __g_TC_sse_converters, source1, source2, destination );
	__measure ( __g_TC_sse_converters );

	printf ( "\n== SIMD Code (AVX) ==\n" );
	__check_valid ( __g_TC_avx_converters, source1, source2, destination );
	__measure ( __g_TC_avx_converters );

	return 0;
}