#ifndef __AUDEAR_DECODERS_H__
#define __AUDEAR_DECODERS_H__

EXTC AEEXP bool AE_registerAudioDecoderCreator ( error_t ( *creator ) ( AEAUDIODECODER ** ) );
EXTC AEEXP void AE_unregisterAudioDecoderCreator ( error_t ( *creator ) ( AEAUDIODECODER** ) );
EXTC AEEXP error_t AE_detectAudioDecoder ( AESTREAM * stream, AEAUDIODECODER ** ret );

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
EXTC AEEXP error_t AE_createMediaFoundationAudioDecoder ( AEAUDIODECODER ** ret );
#endif

#endif