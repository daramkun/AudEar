#ifndef __AUDEAR_PLAYERS_H__
#define __AUDEAR_PLAYERS_H__

#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
EXTC AEEXP AEERROR STDCALL AE_createXAudio2Player ( IXAudio2 * xaudio2, AEBaseAudioPlayer ** player );
EXTC AEEXP AEERROR STDCALL AE_createWASAPIPlayer ( IMMDevice * mmDevice, AUDCLNT_SHAREMODE shareMode, AEBaseAudioPlayer ** player );
#	endif

#endif