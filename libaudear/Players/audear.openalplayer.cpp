#include "../audear.h"

#if ( AE_PLATFORM_WINDOWS && defined ( USE_OPENAL ) ) || !AE_PLATFORM_WINDOWS && !AE_PLATFORM_UWP 

#	if AE_PLATFORM_WINDOWS
#		include <al.h>
#		include <alc.h>
#		pragma comment ( lib, "OpenAL32.lib" )
#	else
#		include <AL/al.h>
#		include <AL/alc.h>
#	endif

EXTC AEEXP error_t AE_createOpenALAudioPlayer ( AEAUDIOPLAYER ** ret )
{
	return AEERROR_NOT_IMPLEMENTED;
}

#endif