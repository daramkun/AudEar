#ifndef __AUDEAR_FFMPEG_H__
#define __AUDEAR_FFMPEG_H__

#include <audear.h>

#if AEWINDOWS
#	if DARAMEEAUDEARFFMPEG_EXPORTS
#		define AEFFEXP										__declspec ( dllexport )
#	else
#		define AEFFEXP										__declspec ( dllimport )
#	endif
#endif

extern "C" AEFFEXP HRESULT __stdcall AE_CreateFFmpegAudioDecoder ( AEAudioDecoder ** decoder );

#endif