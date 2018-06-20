#include "../audear.h"

MMAPI* AE_createMMapiInterface ()
{
	return new MMAPI ();
}
void AE_destroyMMapiInterface ( MMAPI * api )
{
	if ( api == nullptr ) return;
	delete api;
}
IMMDevice* AE_getDefaultDevice ( MMAPI * api )
{
	if ( api == nullptr ) return nullptr;
	return api->operator IMMDevice * ( );
}
size_t AE_getDeviceCount ( MMAPI * api )
{
	if ( api == nullptr ) return 0;
	return api->getRenderDevicesCount ();
}
IMMDevice* AE_getDevice ( MMAPI * api, size_t index )
{
	if ( api == nullptr ) return nullptr;
	return api [ index ];
}

XAudio2 * AE_createXAudio2Interface ()
{
	return new XAudio2 ();
}
void AE_destroyXAudio2Interface ( XAudio2 * xa )
{
	if ( xa == nullptr ) return;
	delete xa;
}
IXAudio2 * AE_getIXAudio2 ( XAudio2 * xa )
{
	if ( xa == nullptr ) return nullptr;
	return xa->operator IXAudio2 * ( );
}
IXAudio2MasteringVoice * AE_getIXAudio2MasteringVoice ( XAudio2 * xa )
{
	if ( xa == nullptr ) return nullptr;
	return xa->operator IXAudio2MasteringVoice * ( );
}
