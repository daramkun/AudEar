#ifndef __AUDEAR_H__
#define __AUDEAR_H__

#include "audear.preprocessor.h"
#include "audear.struct.h"

#define UUID_AEUnknown										DECLSPEC_UUID ( "BB6E6DB3-E3F4-4735-A439-556B2A3F8E56" )
// Implemented IUnknown
class AEEXP UUID_AEUnknown AEUnknown : public IUnknown
{
public:
	AEUnknown ();
	virtual ~AEUnknown ();

public:
#if AEWINDOWS
	virtual HRESULT __stdcall QueryInterface ( const IID & riid, void ** ppvObject );
#endif
	virtual ULONG __stdcall AddRef ();
	virtual ULONG __stdcall Release ();

private:
	ULONG _refCount;
};

#define UUID_AEDecodedSample								DECLSPEC_UUID ( "8B75AD21-A70D-481A-92C5-8DC80B180AC4" )
class AEEXP UUID_AEDecodedSample AEDecodedSample : public AEUnknown
{
#if AEWINDOWS
public:
	virtual HRESULT __stdcall QueryInterface ( const IID & riid, void ** ppvObject );
#endif

public:
	virtual HRESULT __stdcall GetSampleTime ( AETimeSpan * time ) PURE;
	virtual HRESULT __stdcall GetSampleDuration ( AETimeSpan * duration ) PURE;

public:
	virtual HRESULT __stdcall Lock ( void ** buffer, LONGLONG * length ) PURE;
	virtual HRESULT __stdcall Unlock () PURE;
};

#define AE_CreateFileStream(x,y)							SHCreateStreamOnFile ( x, STGM_READ | STGM_SHARE_DENY_WRITE, y )

#define UUID_AEAudioDecoder									DECLSPEC_UUID ( "8D10EA9E-32A1-4B3B-A5DF-E8CF7A7639A5" )
// Audio Decoder Interface
class AEEXP UUID_AEAudioDecoder AEAudioDecoder : public AEUnknown
{
#if AEWINDOWS
public:
	virtual HRESULT __stdcall QueryInterface ( const IID & riid, void ** ppvObject );
#endif

public:
	virtual HRESULT __stdcall Initialize ( IStream * stream, WAVEFORMATEX * outputFormat ) PURE;

public:
	virtual HRESULT __stdcall GetDuration ( AETimeSpan * duration ) PURE;
	virtual HRESULT __stdcall GetWaveFormat ( WAVEFORMATEX ** pwfx ) PURE;

public:
	virtual HRESULT __stdcall SetReadPosition ( const AETimeSpan & pos, AETimeSpan * newPos ) PURE;

public:
	virtual HRESULT __stdcall GetSample ( AEDecodedSample ** sample ) PURE;
};

#if AEWINDOWS
extern "C" AEEXP HRESULT __stdcall AE_CreateMediaFoundationAudioDecoder ( AEAudioDecoder ** decoder );
#endif

#define UUID_AEAudioStream									DECLSPEC_UUID ( "31AB68F4-10AA-47F7-A232-89A21E4B4500" )
// Audio Decoded Buffer Streaming Interface
class AEEXP UUID_AEAudioStream AEAudioStream : public IStream
{
public:
	virtual HRESULT __stdcall GetSourceDecoder ( AEAudioDecoder ** decoder ) PURE;
	virtual HRESULT __stdcall GetCurrentPosition ( AETimeSpan * currentTime ) PURE;

public:
	virtual HRESULT __stdcall Buffering () PURE;
	virtual HRESULT __stdcall FlushBuffer () PURE;
};

extern "C" AEEXP HRESULT __stdcall AE_CreateAudioStream ( AEAudioDecoder * decoder, int maximumBufferSize, AEAudioStream ** stream );

enum AEAudioPlayerState
{
	kAEAPS_Stopped,
	kAEAPS_Playing,
	kAEAPS_Paused,
};

#define UUID_AEAudioPlayer									DECLSPEC_UUID ( "51DA9687-C7B7-4594-AB30-B299BC96E038" )
// Audio Player Interface
class AEEXP UUID_AEAudioPlayer AEAudioPlayer : public AEUnknown
{
#if AEWINDOWS
public:
	virtual HRESULT __stdcall QueryInterface ( const IID & riid, void ** ppvObject );
#endif

public:
	virtual HRESULT __stdcall GetOutputFormat ( WAVEFORMATEX ** format ) PURE;

public:
	virtual HRESULT __stdcall SetSourceStream ( AEAudioStream * stream ) PURE;

public:
	virtual HRESULT __stdcall Play () PURE;
	virtual HRESULT __stdcall Pause () PURE;
	virtual HRESULT __stdcall Stop () PURE;

public:
	virtual HRESULT __stdcall GetPlayingPosition ( AETimeSpan * pos ) PURE;
	virtual HRESULT __stdcall SetPlayingPosition ( const AETimeSpan & pos ) PURE;
	virtual HRESULT __stdcall GetDuration ( AETimeSpan * duration ) PURE;
	virtual HRESULT __stdcall GetPlayerState ( AEAudioPlayerState * state ) PURE;

public:
	virtual HRESULT __stdcall SetVolume ( float volume ) PURE;
	virtual HRESULT __stdcall GetVolume ( float * volume ) PURE;
};

#if AEWINDOWS
#include <xaudio2.h>
extern "C" AEEXP HRESULT __stdcall AE_CreateXAudio2AudioPlayer ( IXAudio2 * xaudio2, AEAudioPlayer ** audioPlayer );
#include <mmdeviceapi.h>
extern "C" AEEXP HRESULT __stdcall AE_CreateWASAPIAudioPlayer ( IMMDevice * device, AUDCLNT_SHAREMODE shareMode, AEAudioPlayer ** audioPlayer );
#endif

#endif