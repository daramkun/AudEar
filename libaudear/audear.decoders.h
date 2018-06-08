#ifndef __AUDEAR_DECODERS_H__
#define __AUDEAR_DECODERS_H__

#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
EXTC error_t AEEXP AE_createMediaFoundationDecoder ( AEBaseAudioDecoder ** decoder );
#	endif

#endif