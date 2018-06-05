#include <stdio.h>
#include <atlbase.h>
#include <audear.h>

#pragma comment ( lib, "libaudear.lib" )
#pragma comment ( lib, "xaudio2.lib" )

class COM
{
public:
	COM () { CoInitialize ( 0 ); }
	~COM () { CoUninitialize (); }
};

class MMAPI
{
public:
	MMAPI ()
	{
		CoCreateInstance ( __uuidof ( MMDeviceEnumerator ), nullptr, CLSCTX_ALL,
			__uuidof( IMMDeviceEnumerator ), ( void ** ) &devEnum );
	}

public:
	operator IMMDevice* ( )
	{
		IMMDevice * dev;
		devEnum->GetDefaultAudioEndpoint ( eRender, eConsole, &dev );
		return dev;
	}

private:
	CComPtr<IMMDeviceEnumerator> devEnum;
};

int main ( int argc, char * argv [] )
{
#define FILENAME TEXT ( "F:\\Multimedia\\Music\\KPOP&ROCK\\厩悼孤瘤记\\厩悼孤瘤记-04-给积变 么.mp3" )
//#define FILENAME TEXT ( "F:\\Multimedia\\Music\\Game\\DJMAX RESPECT\\DISK 1\\18. BlackCat.mp3" )

	COM com;

	/*CComPtr<IXAudio2> xaudio2;
	if ( FAILED ( XAudio2Create ( &xaudio2 ) ) )
		return -1;

	IXAudio2MasteringVoice * masteringVoice;
	if ( FAILED ( xaudio2->CreateMasteringVoice ( &masteringVoice ) ) )
		return -2;

	xaudio2->StartEngine ();

	CComPtr<AEAudioPlayer> audioPlayer;
	if ( FAILED ( AE_CreateXAudio2AudioPlayer ( xaudio2, &audioPlayer ) ) )
		return -3;/**/

	/**/MMAPI mmapi;
	CComPtr<AEAudioPlayer> audioPlayer;
	if ( FAILED ( AE_CreateWASAPIAudioPlayer ( mmapi, AUDCLNT_SHAREMODE_SHARED, &audioPlayer ) ) )
		return -1;/**/

	WAVEFORMATEX * outputFormat;
	audioPlayer->GetOutputFormat ( &outputFormat );

	CComPtr<AEAudioDecoder> decoder;
	if ( FAILED ( AE_CreateMediaFoundationAudioDecoder ( &decoder ) ) )
		return -4;

	CComPtr<IStream> fileStream;
	if ( FAILED ( AE_CreateFileStream ( FILENAME, &fileStream ) ) )
		return -5;

	if ( FAILED ( decoder->Initialize ( fileStream, outputFormat ) ) )
		return -6;

	//if ( outputFormat != nullptr )
	//	free ( outputFormat );

	CComPtr<AEAudioStream> audioStream;
	if ( FAILED ( AE_CreateAudioStream ( decoder,
		/*10485760/**/				// 10MB
		/*2097152/**/				// 2MB
		/*524288/**/				// 512KB
		outputFormat->nAvgBytesPerSec
		, &audioStream ) ) )
		return -7;

	if ( FAILED ( audioPlayer->SetSourceStream ( audioStream ) ) )
		return -8;

	if ( FAILED ( audioPlayer->Play () ) )
		return -9;

	while ( true )
	{
		AEAudioPlayerState state;
		if ( FAILED ( audioPlayer->GetPlayerState ( &state ) ) )
			return -10;

		if ( state != kAEAPS_Playing )
			break;

		AETimeSpan pos, duration;
		audioPlayer->GetPlayingPosition ( &pos );
		audioPlayer->GetDuration ( &duration );

		printf ( "%s/%s\n", ( ( std::string ) pos ).c_str (), ( ( std::string ) duration ).c_str () );

		Sleep ( 100 );
	}

	/*xaudio2->StopEngine ();

	masteringVoice->DestroyVoice ();/**/

	return 0;
}