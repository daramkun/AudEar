#include "../libaudear/audear.preprocessors.h"
#include "../libaudear/audear.struct.h"
#include "../libaudear/InnerUtilities/TypeConverter.hpp"
#include "../libaudear/InnerUtilities/HighResolutionTimer.hpp"

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
#define SAMPLE_SOURCE_TYPE									int24_t
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
		printf ( "F(Faild call S->D)\n" );
		return;
	}

	if ( !converters.get_converter<SAMPLE_DESTINATION_TYPE, SAMPLE_SOURCE_TYPE> () (
		destination, source2,
		SAMPLE_SIZE * SAMPLE_CHANNELS * sizeof ( SAMPLE_DESTINATION_TYPE ),
		SAMPLE_SIZE * SAMPLE_CHANNELS * sizeof ( SAMPLE_SOURCE_TYPE ) ) )
	{
		printf ( "F(Faild call D->S)\n" );
		return;
	}

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
				int sm = pow ( 2, sizeof ( SAMPLE_SOURCE_TYPE ) * 8 - 1 ) - 1;
				int dm = pow ( 2, sizeof ( SAMPLE_DESTINATION_TYPE ) * 8 - 1 ) - 1;
				if ( abs ( source1 [ i ] - source2 [ i ] ) > ( max ( sm, dm ) / min ( sm, dm ) ) )
					diff = true;
			}

			if ( diff )
			{
				printf ( "F(%f to %f, Diff: %f)\n",
					( float ) source1 [ i ], ( float ) source2 [ i ],
					( float ) ( source1 [ i ] - source2 [ i ] ) );
				return;
			}
		}
	}

	printf ( "T\n" );
}

void __measure ( const __TC_SAMPLE_CONVERTERS & converters )
{
	static int64_t freq = __HRT_GetFrequency ();
	int64_t start, end;

	using namespace std;

	shared_ptr<SAMPLE_SOURCE_TYPE []> source ( new SAMPLE_SOURCE_TYPE [ SAMPLE_SIZE * SAMPLE_CHANNELS ] );
	shared_ptr<SAMPLE_DESTINATION_TYPE []> destination ( new SAMPLE_DESTINATION_TYPE [ SAMPLE_SIZE * SAMPLE_CHANNELS ] );
	
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

	if ( std::is_same<SAMPLE_SOURCE_TYPE, float>::value )
	{
		for ( int i = 0; i < SAMPLE_SIZE * SAMPLE_CHANNELS; ++i )
			source1 [ i ] = rand () / ( float ) INT_MAX;
	}

	printf ( "\n== Plain C++ Code ==\n" );
	__check_valid ( __g_TC_plain_converters, source1, source2, destination );
	__measure ( __g_TC_plain_converters );

	printf ( "\n== SIMD Code ==\n" );
	__check_valid ( __g_TC_sse_converters, source1, source2, destination );
	__measure ( __g_TC_sse_converters );

	return 0;
}