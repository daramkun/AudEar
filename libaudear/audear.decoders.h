#ifndef __AUDEAR_DECODERS_H__
#define __AUDEAR_DECODERS_H__

#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
EXTC AEEXP AEERROR STDCALL AE_createMediaFoundationDecoder ( AEBaseAudioDecoder ** decoder );
#	endif

#endif