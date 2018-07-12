#include "audear.osscodec.h"

EXTC AEOEXP void AE_OSS_init ()
{

}

#pragma comment ( lib, "libaudear.lib" )

static void register_oss_decoder ()
{
	AE_registerAudioDecoderCreator ( AE_createLimeMP3AudioDecoder );
	AE_registerAudioDecoderCreator ( AE_createOggVorbisAudioDecoder );
	AE_registerAudioDecoderCreator ( AE_createOggOpusAudioDecoder );
	AE_registerAudioDecoderCreator ( AE_createFLACAudioDecoder );
	AE_registerAudioDecoderCreator ( AE_createOggFLACAudioDecoder );
	AE_registerAudioDecoderCreator ( AE_createWAVAudioDecoder );
}

static void unregister_oss_decoder ()
{
	AE_unregisterAudioDecoderCreator ( AE_createWAVAudioDecoder );
	AE_unregisterAudioDecoderCreator ( AE_createOggFLACAudioDecoder );
	AE_unregisterAudioDecoderCreator ( AE_createFLACAudioDecoder );
	AE_unregisterAudioDecoderCreator ( AE_createOggOpusAudioDecoder );
	AE_unregisterAudioDecoderCreator ( AE_createOggVorbisAudioDecoder );
	AE_unregisterAudioDecoderCreator ( AE_createLimeMP3AudioDecoder );
}

#if ( AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP ) && AE_COMPILER_MSVC

#include <Windows.h>

EXTC int APIENTRY DllMain ( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{
	if ( dwReason == DLL_PROCESS_ATTACH )
	{
		register_oss_decoder ();
	}
	else if ( dwReason == DLL_PROCESS_DETACH )
	{
		unregister_oss_decoder ();
	}
	return 1;
}

#elif AE_COMPILER_GCC || AE_COMPILER_CLANG

__attribute__ ( ( constructor ) ) static void __constructor_audear_oss__ ()
{
	register_oss_decoder ();
}

__attribute__ ( ( destructor ) ) static void __destructor_audear_oss__ ()
{
	unregister_oss_decoder ();
}

#endif