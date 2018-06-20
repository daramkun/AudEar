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

EXTC AEOSSCEXP AEERROR STDCALL AE_createLameMp3Decoder ( AEBaseAudioDecoder ** decoder );
EXTC AEOSSCEXP AEERROR STDCALL AE_createOggOpusDecoder ( AEBaseAudioDecoder ** decoder );
EXTC AEOSSCEXP AEERROR STDCALL AE_createOggVorbisDecoder ( AEBaseAudioDecoder ** decoder );
EXTC AEOSSCEXP AEERROR STDCALL AE_createOggFLACDecoder ( AEBaseAudioDecoder ** decoder );
EXTC AEOSSCEXP AEERROR STDCALL AE_createFLACDecoder ( AEBaseAudioDecoder ** decoder );
EXTC AEOSSCEXP AEERROR STDCALL AE_createM4AAACDecoder ( AEBaseAudioDecoder ** decoder );

#endif