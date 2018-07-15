#ifndef __AUDEAR_PLAYERS_H__
#define __AUDEAR_PLAYERS_H__

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
typedef enum AEEXP
{
	AEWASAPISM_SHARED,
	AEWASAPISM_EXCLUSIVE,
} AEWASAPISHAREMODE;

EXTC AEEXP error_t AE_createWASAPIAudioPlayer ( IUnknown * device, AEWASAPISHAREMODE shareMode, AEAUDIOPLAYER ** ret );
EXTC AEEXP error_t AE_createWASAPIAudioPlayerWithoutParameters ( AEAUDIOPLAYER ** ret );

EXTC AEEXP error_t AE_createXAudio2AudioPlayer ( int32_t processor, AEAUDIOPLAYER ** ret );
EXTC AEEXP error_t AE_createXAudio2AudioPlayerWithoutParameters ( AEAUDIOPLAYER ** ret );
#endif

#if ( defined ( USE_OPENAL ) && USE_OPENAL )
EXTC AEEXP error_t AE_createOpenALAudioPlayer ( const char * deviceName, AEAUDIOPLAYER ** ret );
#endif

#endif