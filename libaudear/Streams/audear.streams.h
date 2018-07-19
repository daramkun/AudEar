#ifndef __AUDEAR_STREAMS_H__
#define __AUDEAR_STREAMS_H__

EXTC AEEXP error_t AE_createFileStream ( const char * filename, AESTREAM ** ret );

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
EXTC AEEXP HRESULT AE_convertWindowsComStream ( AESTREAM * stream, IStream ** istream );
#endif

EXTC AEEXP error_t AE_createBufferedAudioStream ( AEAUDIODECODER * decoder, AEAUDIOSTREAM ** ret );
EXTC AEEXP error_t AE_createWholeAudioStream ( AEAUDIODECODER * decoder, AEAUDIOSTREAM ** ret );

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
EXTC AEEXP error_t AE_createDmoResamplerAudioStream ( AEAUDIOSTREAM * stream, WAVEFORMATEX * pwfx, AEAUDIOSTREAM ** ret );
#endif

EXTC AEEXP error_t AE_createPCMToIEEEFloatAudioStream ( AEAUDIOSTREAM * stream, AEAUDIOSTREAM ** ret );
EXTC AEEXP error_t AE_createIEEEFloatToPCMAudioStream ( AEAUDIOSTREAM * stream, int bps, AEAUDIOSTREAM ** ret );
EXTC AEEXP error_t AE_createPCMToPCMAudioStream ( AEAUDIOSTREAM * stream, int bps, AEAUDIOSTREAM ** ret );
EXTC AEEXP error_t AE_createConverterAudioStream ( AEAUDIOSTREAM * stream, AEAUDIOFORMAT af, int bps, AEAUDIOSTREAM ** ret );

EXTC AEEXP error_t AE_createMultiChannelsToMonoAudioStream ( AEAUDIOSTREAM * stream, AEAUDIOSTREAM ** ret );

#endif