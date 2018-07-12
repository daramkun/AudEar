#ifndef __AUDEAR_OSSCODEC_H__
#define __AUDEAR_OSSCODEC_H__

#include <audear.h>

#if AE_PLATFORM_WINDOWS
#	if defined ( LIBAUDEAROSSCODEC_EXPORTS ) && LIBAUDEAROSSCODEC_EXPORTS
#		define AEOEXP										__declspec ( dllexport )
#	else
#		define AEOEXP										__declspec ( dllimport )
#	endif
#else
#	define AEOEXP
#endif

extern "C"
{
	EXTC AEOEXP void AE_OSS_init ();

	EXTC AEOEXP error_t AE_createLimeMP3AudioDecoder ( AEAUDIODECODER ** ret );
	EXTC AEOEXP error_t AE_createOggVorbisAudioDecoder ( AEAUDIODECODER ** ret );
	EXTC AEOEXP error_t AE_createOggOpusAudioDecoder ( AEAUDIODECODER ** ret );
	EXTC AEOEXP error_t AE_createOggFLACAudioDecoder ( AEAUDIODECODER ** ret );
	EXTC AEOEXP error_t AE_createFLACAudioDecoder ( AEAUDIODECODER ** ret );
	EXTC AEOEXP error_t AE_createWAVAudioDecoder ( AEAUDIODECODER ** ret );
	//EXTC AEOEXP error_t AE_createWavPackAudioDecoder ( AEAUDIODECODER ** ret );
	//EXTC AEOEXP error_t AE_createISOBMFFAACAudioDecoder ( AEAUDIODECODER ** ret );
	//EXTC AEOEXP error_t AE_createMatroskaAACAudioDecoder ( AEAUDIODECODER ** ret );
	//EXTC AEOEXP error_t AE_createAIFFAACAudioDecoder ( AEAUDIODECODER ** ret );
	//EXTC AEOEXP error_t AE_createADTSAACAudioDecoder ( AEAUDIODECODER ** ret );
	//EXTC AEOEXP error_t AE_createWMAAudioDecoder ( AEAUDIODECODER ** ret );
}

#endif