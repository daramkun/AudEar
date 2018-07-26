#include "../libaudear/audear.preprocessors.h"
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
#define SAMPLE_SOURCE_TYPE									int16_t
#define SAMPLE_DESTINATION_TYPE								float

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

	__TypeConverterFunction converter = __g_TC_plain_converters.get_converter<SAMPLE_SOURCE_TYPE, SAMPLE_DESTINATION_TYPE> ();

	printf ( "\n== Plain C++ Code ==\n" );
	__measure ( __g_TC_plain_converters );

	printf ( "\n== SIMD Code ==\n" );
	__measure ( __g_TC_sse_converters );

	return 0;
}