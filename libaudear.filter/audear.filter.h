#ifndef __AUDEAR_FILTER_H__
#define __AUDEAR_FILTER_H__

#include <audear.h>

#if AE_PLATFORM_WINDOWS
#	if defined ( LIBAUDEARFILTER_EXPORTS ) && LIBAUDEARFILTER_EXPORTS
#		define AEFEXP										__declspec ( dllexport )
#	else
#		define AEFEXP										__declspec ( dllimport )
#	endif
#else
#	define AEFEXP
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	typedef struct AEFEXP _AEBIQUADFILTER
	{
		double a0, a1, a2, a3, a4;
		double x1 [ 12 ], x2 [ 12 ], y1 [ 12 ], y2 [ 12 ];
	} AEBIQUADFILTER;

	EXTC AEFEXP float AEBIQUADFILTER_process ( AEBIQUADFILTER * f, float input, int ch );

	EXTC AEFEXP AEBIQUADFILTER AE_initializeHighPassFilter ( int samplerate, double freq, double q );
	EXTC AEFEXP AEBIQUADFILTER AE_initializeHighShelfFilter ( int samplerate, double freq, float shelfSlope, double gaindb );
	EXTC AEFEXP AEBIQUADFILTER AE_initializeLowPassFilter ( int samplerate, double freq, double q );
	EXTC AEFEXP AEBIQUADFILTER AE_initializeLowShelfFilter ( int samplerate, double freq, float shelfSlope, double gaindb );
	EXTC AEFEXP AEBIQUADFILTER AE_initializeAllPassFilter ( int samplerate, double freq, float q );
	EXTC AEFEXP AEBIQUADFILTER AE_initializeNotchFilter ( int samplerate, double freq, double q );
	EXTC AEFEXP AEBIQUADFILTER AE_initializePeakEqFilter ( int samplerate, double freq, double bandwidth, double peakGainDB );
	EXTC AEFEXP AEBIQUADFILTER AE_initializeBandPassFilterCSG ( int samplerate, double freq, double q );
	EXTC AEFEXP AEBIQUADFILTER AE_initializeBandPassFilterCPG ( int samplerate, double freq, double q );

	typedef struct AEFEXP
	{
		AEBIQUADFILTER filters [ 16 ];
		int8_t filters_size;
	} AEFILTERCOLLECTION;

	typedef enum AEFEXP
	{
		AEEQP_NONE,
		AEEQP_ACOUSTIC,
		AEEQP_DANCE,
		AEEQP_JAZZ,
		AEEQP_ELECTRONIC,
		AEEQP_POP,
		AEEQP_ROCK,
		AEEQP_HIPHOP,
		AEEQP_CLASSICAL,
		AEEQP_PIANO,
	} AEEQUALIZERPRESET;

	EXTC AEFEXP AEFILTERCOLLECTION AE_initializeSingleItemFilterCollection ( AEBIQUADFILTER filter );
	EXTC AEFEXP AEFILTERCOLLECTION AE_initializeEqualizerFilterCollection ( int samplerate, double bandwidth, AEEQUALIZERPRESET preset );
	EXTC AEFEXP AEFILTERCOLLECTION AE_initializeEqualizerFilterCollectionWithGainDB ( int samplerate, double bandwidth, double * gainDBs );

	EXTC AEFEXP error_t AE_createFilterAudioStream ( AEAUDIOSTREAM * stream, AEFILTERCOLLECTION * collection, bool collectionConst, AEAUDIOSTREAM ** ret );

	typedef enum
	{
		AESC_NONE = 0,
		AESC_CH1 = 1 << 0,
		AESC_CH2 = 1 << 1,
		AESC_CH3 = 1 << 2,
		AESC_CH4 = 1 << 3,
		AESC_CH5 = 1 << 4,
		AESC_CH6 = 1 << 5,
		AESC_CH7 = 1 << 6,
		AESC_CH8 = 1 << 7,
		AESC_CH9 = 1 << 8,
		AESC_CH10 = 1 << 9,
		AESC_AVGALL = 0xffffffff,
	} AESPECTRUMCHANNELS;

	EXTC AEFEXP error_t AE_createFrequencySpectrumAudioStream ( AEAUDIOSTREAM * stream, void ( *callback ) ( AESPECTRUMCHANNELS ch, float * freqs, int freqsCount ), AESPECTRUMCHANNELS ch, AEAUDIOSTREAM ** ret );
	EXTC AEFEXP error_t AE_createDecibelSpectrumAudioStream ( AEAUDIOSTREAM * stream, void ( *callback ) ( AESPECTRUMCHANNELS ch, float * dbs, int dbsCount ), AESPECTRUMCHANNELS ch, AEAUDIOSTREAM ** ret );
#ifdef __cplusplus
}
#endif

#endif