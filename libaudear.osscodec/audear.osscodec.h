#ifndef __AUDEAR_OSSCODEC_H__
#define __AUDEAR_OSSCODEC_H__

#include <audear.h>

#if AE_PLATFORM_WINDOWS
#	if defined ( LIBAUDEAROSSCODEC_EXPORTS )
#		define AEOSSCEXP																			__declspec ( dllexport )
#	else
#		define AEOSSCEXP																			__declspec ( dllimport )
#	endif
#endif

EXTC error_t AEOSSCEXP AE_createLameMp3Decoder ( AEBaseAudioDecoder ** decoder );
EXTC error_t AEOSSCEXP AE_createOggOpusDecoder ( AEBaseAudioDecoder ** decoder );
EXTC error_t AEOSSCEXP AE_createOggVorbisDecoder ( AEBaseAudioDecoder ** decoder );
EXTC error_t AEOSSCEXP AE_createOggFLACDecoder ( AEBaseAudioDecoder ** decoder );
EXTC error_t AEOSSCEXP AE_createFLACDecoder ( AEBaseAudioDecoder ** decoder );
EXTC error_t AEOSSCEXP AE_createM4AAACDecoder ( AEBaseAudioDecoder ** decoder );

#endif