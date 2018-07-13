#include "../audear.h"

static void register_decoder ()
{
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
	AE_registerAudioDecoderCreator ( AE_createMediaFoundationAudioDecoder );
#endif
}

static void unregister_decoder ()
{
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
	AE_unregisterAudioDecoderCreator ( AE_createMediaFoundationAudioDecoder );
#endif
}

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP

#include <Windows.h>
#include <VersionHelpers.h>

EXTC int APIENTRY DllMain ( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{
	if ( dwReason == DLL_PROCESS_ATTACH )
	{
		if ( !IsWindows7OrGreater () )
			return 0;

		register_decoder ();
	}
	else if ( dwReason == DLL_PROCESS_DETACH )
	{
		unregister_decoder ();
	}
	return 1;
}

#elif AE_PLATFORM_UNIX || AE_PLATFORM_MACOS || AE_PLATFORM_IOS || AE_PLATFORM_ANDROID

__attribute__ ( ( constructor ) ) static void __constructor_audear__ ()
{
	register_decoder ();
}

__attribute__ ( ( destructor ) ) static void __destructor_audear__ ()
{
	unregister_decoder ();
}

#endif

class COM
{
public:
	COM () { CoInitialize ( nullptr ); }
	~COM () { CoUninitialize (); }
} g_com;

EXTC AEEXP void AE_init ()
{
	
}
