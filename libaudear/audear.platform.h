#ifndef __AUDEAR_PLATFORM_H__
#define __AUDEAR_PLATFORM_H__

#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
class COM
{
public:
	inline COM ( DWORD init = COINIT_APARTMENTTHREADED ) { CoInitializeEx ( nullptr, init ); }
	inline ~COM () { CoUninitialize (); }
};
#		include <atlbase.h>

#		if defined ( NTDDI_WIN6 )
#			include <mmdeviceapi.h>
class MMAPI
{
public:
	inline MMAPI ()
	{
		if ( FAILED ( CoCreateInstance ( __uuidof ( MMDeviceEnumerator ), nullptr, CLSCTX_ALL,
			__uuidof( IMMDeviceEnumerator ), ( void ** ) &devEnum ) ) )
			return;
		devEnum->EnumAudioEndpoints ( eRender, eConsole, &collection );
	}

public:
	inline size_t getRenderDevicesCount ()
	{
		if ( collection == nullptr )
			return 0;

		UINT temp;
		if ( FAILED ( collection->GetCount ( &temp ) ) )
			return 0;
		return temp;
	}

public:
	inline operator IMMDevice* ( )
	{
		if ( devEnum == nullptr )
			return nullptr;
		IMMDevice * device;
		if ( FAILED ( devEnum->GetDefaultAudioEndpoint ( eRender, eConsole, &device ) ) )
			return nullptr;
		return device;
	}

	inline IMMDevice* operator [] ( int index )
	{
		if ( collection == nullptr )
			return nullptr;
		IMMDevice * device;
		if ( FAILED ( collection->Item ( index, &device ) ) )
			return nullptr;
		return device;
	}

private:
	CComPtr<IMMDeviceEnumerator> devEnum;
	CComPtr<IMMDeviceCollection> collection;
};

EXTC AEEXP MMAPI* STDCALL AE_createMMapiInterface ();
EXTC AEEXP void STDCALL AE_destroyMMapiInterface ( MMAPI * api );
EXTC AEEXP IMMDevice* STDCALL AE_getDefaultDevice ( MMAPI * api );
EXTC AEEXP size_t STDCALL AE_getDeviceCount ( MMAPI * api );
EXTC AEEXP IMMDevice* STDCALL AE_getDevice ( MMAPI * api, size_t index );

#			include <xaudio2.h>
class XAudio2
{
public:
	inline XAudio2 ()
	{
		XAudio2Create ( &xaudio2 );
		xaudio2->CreateMasteringVoice ( &masteringVoice );
		xaudio2->StartEngine ();
	}
	inline ~XAudio2 ()
	{
		xaudio2->StopEngine ();
		masteringVoice->DestroyVoice ();
	}

public:
	inline operator IXAudio2* () { return xaudio2; }
	inline operator IXAudio2MasteringVoice* () { return masteringVoice; }

private:
	CComPtr<IXAudio2> xaudio2;
	IXAudio2MasteringVoice * masteringVoice;
};

EXTC AEEXP XAudio2* STDCALL AE_createXAudio2Interface ();
EXTC AEEXP void STDCALL AE_destroyXAudio2Interface ( XAudio2 * xa );
EXTC AEEXP IXAudio2* STDCALL AE_getIXAudio2 ( XAudio2 * xa );
EXTC AEEXP IXAudio2MasteringVoice* STDCALL AE_getIXAudio2MasteringVoice ( XAudio2 * xa );

#			include <mfapi.h>
class MediaFoundation
{
public:
	inline MediaFoundation () { hr = MFStartup ( MF_VERSION ); }
	inline ~MediaFoundation () { MFShutdown (); }

public:
	inline HRESULT getStartupResult () const { return hr; }

private:
	HRESULT hr;
};

#		endif
#	endif

#endif