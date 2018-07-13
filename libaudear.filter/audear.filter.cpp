#include "audear.filter.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265359
#endif

void __set_coefficient ( AEBIQUADFILTER & f, double aa0, double aa1, double aa2, double b0, double b1, double b2 )
{
	double invaa0 = 1 / aa0;
	f.a0 = b0 * invaa0;
	f.a1 = b1 * invaa0;
	f.a2 = b2 * invaa0;
	f.a3 = aa1 * invaa0;
	f.a4 = aa2 * invaa0;
}

float AEBIQUADFILTER_process ( AEBIQUADFILTER * f, float input, int ch )
{
	double result = f->a0 * input
		+ f->a1 * f->x1 [ ch ]
		+ f->a2 * f->x2 [ ch ]
		- f->a3 * f->y1 [ ch ]
		- f->a4 * f->y2 [ ch ];
	f->x2 [ ch ] = f->x1 [ ch ];
	f->x1 [ ch ] = input;

	f->y2 [ ch ] = f->y1 [ ch ];
	f->y1 [ ch ] = result;

	return ( float ) result;
}

AEBIQUADFILTER AE_initializeHighPassFilter ( int samplerate, double freq, double q )
{
	AEBIQUADFILTER f = {};

	double w0 = 2 * M_PI * freq / samplerate;
	double cosw0 = cos ( w0 );
	double alpha = sin ( w0 ) / ( 2 * q );

	double b0 = ( 1 + cosw0 ) / 2;
	double b1 = -( 1 + cosw0 );
	double b2 = ( 1 + cosw0 ) / 2;
	double aa0 = 1 + alpha;
	double aa1 = -2 * cosw0;
	double aa2 = 1 - alpha;

	__set_coefficient ( f, aa0, aa1, aa2, b0, b1, b2 );

	return f;
}

AEBIQUADFILTER AE_initializeHighShelfFilter ( int samplerate, double freq, float shelfSlope, double gaindb )
{
	AEBIQUADFILTER f = {};

	double w0 = 2 * M_PI * freq / samplerate;
	double cosw0 = cos ( w0 );
	double sinw0 = sin ( w0 );
	double a = pow ( 10, gaindb / 40 );
	double alpha = sinw0 / 2 * sqrt ( ( a + 1 / a ) * ( 1 / shelfSlope - 1 ) + 2 );
	double temp = 2 * sqrt ( a ) * alpha;

	double b0 = a * ( ( a + 1 ) + ( a - 1 ) * cosw0 + temp );
	double b1 = -2 * a * ( ( a - 1 ) + ( a + 1 ) * cosw0 );
	double b2 = a * ( ( a + 1 ) + ( a - 1 ) * cosw0 - temp );
	double aa0 = ( a + 1 ) - ( a - 1 ) * cosw0 + temp;
	double aa1 = 2 * ( ( a - 1 ) - ( a + 1 ) * cosw0 );
	double aa2 = ( a + 1 ) - ( a - 1 ) * cosw0 - temp;

	__set_coefficient ( f, aa0, aa1, aa2, b0, b1, b2 );

	return f;
}

AEBIQUADFILTER AE_initializeLowPassFilter ( int samplerate, double freq, double q )
{
	AEBIQUADFILTER f = {};

	double w0 = 2 * M_PI * freq / samplerate;
	double cosw0 = cos ( w0 );
	double alpha = sin ( w0 ) / ( 2 * q );

	double b0 = ( 1 - cosw0 ) / 2;
	double b1 = 1 - cosw0;
	double b2 = ( 1 - cosw0 ) / 2;
	double aa0 = 1 + alpha;
	double aa1 = -2 * cosw0;
	double aa2 = 1 - alpha;

	__set_coefficient ( f, aa0, aa1, aa2, b0, b1, b2 );

	return f;
}

AEBIQUADFILTER AE_initializeLowShelfFilter ( int samplerate, double freq, float shelfSlope, double gaindb )
{
	AEBIQUADFILTER f = {};

	double w0 = 2 * M_PI * freq / samplerate;
	double cosw0 = cos ( w0 );
	double sinw0 = sin ( w0 );
	double a = pow ( 10, gaindb / 40 );
	double alpha = sinw0 / 2 * sqrt ( ( a + 1 / a ) * ( 1 / shelfSlope - 1 ) + 2 );
	double temp = 2 * sqrt ( a ) * alpha;

	double b0 = a * ( ( a + 1 ) - ( a - 1 ) * cosw0 + temp );
	double b1 = 2 * a * ( ( a - 1 ) - ( a + 1 ) * cosw0 );
	double b2 = a * ( ( a + 1 ) - ( a - 1 ) * cosw0 - temp );
	double aa0 = ( a + 1 ) + ( a - 1 ) * cosw0 + temp;
	double aa1 = -2 * ( ( a - 1 ) + ( a + 1 ) * cosw0 );
	double aa2 = ( a + 1 ) + ( a - 1 ) * cosw0 - temp;

	__set_coefficient ( f, aa0, aa1, aa2, b0, b1, b2 );

	return f;
}

AEBIQUADFILTER AE_initializeAllPassFilter ( int samplerate, double freq, float q )
{
	AEBIQUADFILTER f = {};

	double w0 = 2 * M_PI * freq / samplerate;
	double cosw0 = cos ( w0 );
	double sinw0 = sin ( w0 );
	double alpha = sinw0 / ( 2 * q );

	double b0 = 1 - alpha;
	double b1 = -2 * cosw0;
	double b2 = 1 + alpha;
	double aa0 = 1 + alpha;
	double aa1 = -2 * cosw0;
	double aa2 = 1 - alpha;

	__set_coefficient ( f, aa0, aa1, aa2, b0, b1, b2 );

	return f;
}

AEBIQUADFILTER AE_initializeNotchFilter ( int samplerate, double freq, double q )
{
	AEBIQUADFILTER f = {};

	double w0 = 2 * M_PI * freq / samplerate;
	double cosw0 = cos ( w0 );
	double sinw0 = sin ( w0 );
	double alpha = sinw0 / ( 2 * q );

	double b0 = 1;
	double b1 = -2 * cosw0;
	double b2 = 1;
	double aa0 = 1 + alpha;
	double aa1 = -2 * cosw0;
	double aa2 = 1 - alpha;

	__set_coefficient ( f, aa0, aa1, aa2, b0, b1, b2 );

	return f;
}

AEBIQUADFILTER AE_initializePeakEqFilter ( int samplerate, double freq, double bandwidth, double peakGainDB )
{
	AEBIQUADFILTER f = {};

	double w0 = 2 * M_PI * freq / samplerate;
	double cosw0 = cos ( w0 );
	double sinw0 = sin ( w0 );
	double alpha = sinw0 / ( 2 * bandwidth );
	double a = pow ( 10, peakGainDB / 40 );

	double b0 = 1 + alpha * a;
	double b1 = -2 * cosw0;
	double b2 = 1 - alpha * a;
	double aa0 = 1 + alpha / a;
	double aa1 = -2 * cosw0;
	double aa2 = 1 - alpha / a;

	__set_coefficient ( f, aa0, aa1, aa2, b0, b1, b2 );

	return f;
}

AEBIQUADFILTER AE_initializeBandPassFilterCSG ( int samplerate, double freq, double q )
{
	AEBIQUADFILTER f = {};

	double w0 = 2 * M_PI * freq / samplerate;
	double cosw0 = cos ( w0 );
	double sinw0 = sin ( w0 );
	double alpha = sinw0 / ( 2 * q );

	double b0 = sinw0 / 2;
	double b1 = 0;
	double b2 = -sinw0 / 2;
	double aa0 = 1 + alpha;
	double aa1 = -2 * cosw0;
	double aa2 = 1 - alpha;

	__set_coefficient ( f, aa0, aa1, aa2, b0, b1, b2 );

	return f;
}

AEBIQUADFILTER AE_initializeBandPassFilterCPG ( int samplerate, double freq, double q )
{
	AEBIQUADFILTER f = {};

	double w0 = 2 * M_PI * freq / samplerate;
	double cosw0 = cos ( w0 );
	double sinw0 = sin ( w0 );
	double alpha = sinw0 / ( 2 * q );

	double b0 = 1 - alpha;
	double b1 = -2 * cosw0;
	double b2 = 1 + alpha;
	double aa0 = 1 + alpha;
	double aa1 = -2 * cosw0;
	double aa2 = 1 - alpha;

	__set_coefficient ( f, aa0, aa1, aa2, b0, b1, b2 );

	return f;
}

EXTC AEFEXP AEFILTERCOLLECTION AE_initializeSingleItemFilterCollection ( AEBIQUADFILTER filter )
{
	AEFILTERCOLLECTION filterCollection = {};
	filterCollection.filters [ 0 ] = filter;
	filterCollection.filters_size = 1;
	return filterCollection;
}

int equalizerFrequencies [ 10 ] = { 32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };

double equalizerPresetGainsDatabase [] [ 10 ] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 5, 5, 4, 1, 2, 2, 4, 5, 4, 3 },
	{ 4, 7, 5, 1, 2, 3, 5, 4, 3, 1 },
	{ 4, 3, 2, 3, -2, -2, 0, 2, 3, 4 },
	{ 4, 3, 2, 0, -2, 3, 2, 2, 4, 5 },
	{ -2, -1, 0, 2, 3, 3, 2, 1, -1, -2 },
	{ 4, 3, 2, 1, 0, 0, 1, 2, 3, 4 },
	{ 4, 3, 1, 2, 0, 0, 2, 0, 2, 3 },
	{ 5, 4, 3, 3, -1, -1, 0, 2, 3, 4 },
	{ 3, 2, 0,3, 3, 2, 3, 4, 3, 4 },
};

EXTC AEFEXP AEFILTERCOLLECTION AE_initializeEqualizerFilterCollection ( int samplerate, double bandwidth, AEEQUALIZERPRESET preset )
{
	AEFILTERCOLLECTION filterCollection = {};
	for ( int i = 0; i < 10; ++i )
		filterCollection.filters [ i ] = AE_initializePeakEqFilter ( samplerate, equalizerFrequencies [ i ],
			bandwidth, equalizerPresetGainsDatabase [ preset ] [ i ] );
	filterCollection.filters_size = 10;
	return filterCollection;
}

EXTC AEFEXP AEFILTERCOLLECTION AE_initializeEqualizerFilterCollectionWithGainDB ( int samplerate, double bandwidth, double * gainDBs )
{
	AEFILTERCOLLECTION filterCollection = {};
	for ( int i = 0; i < 10; ++i )
		filterCollection.filters [ i ] = AE_initializePeakEqFilter ( samplerate, equalizerFrequencies [ i ],
			bandwidth, gainDBs [ i ] );
	filterCollection.filters_size = 10;
	return filterCollection;
}
