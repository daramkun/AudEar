#include <audear.h>
#pragma comment ( lib, "libaudear.lib" )
#pragma comment ( lib, "XAudio2.lib" )
#pragma comment ( lib, "mf.lib" )
#pragma comment ( lib, "mfplat.lib" )

int main ( void )
{
#define FILENAME TEXT ( "F:\\Multimedia\\Music\\KPOP&ROCK\\厩悼孤瘤记\\厩悼孤瘤记-04-给积变 么.mp3" )
//#define FILENAME TEXT ( "F:\\Multimedia\\Music\\Game\\DJMAX RESPECT\\DISK 1\\18. BlackCat.mp3" )
//#define FILENAME TEXT ( "Test.mp3" )

	COM com;
	MediaFoundation mf;
	
	/*XAudio2 xaudio2;
	AEAutoPtr<AEBaseAudioPlayer> audioPlayer;
	if ( FAILED ( AE_createXAudio2Player ( xaudio2, &audioPlayer ) ) )
	return -1;/**/

	/**/MMAPI mmapi;
	AEAutoPtr<AEBaseAudioPlayer> audioPlayer;
	if ( FAILED ( AE_createWASAPIPlayer ( mmapi, AUDCLNT_SHAREMODE_SHARED, &audioPlayer ) ) )
		return -1;/**/

	AEAutoPtr<AEBaseStream> fileStream;
	if ( FAILED ( AE_createFileStream ( FILENAME, true, &fileStream ) ) )
		return -2;

	AEAutoPtr<AEBaseAudioDecoder> decoder;
	if ( FAILED ( AE_createMediaFoundationDecoder ( &decoder ) ) )
		return -3;
	if ( FAILED ( decoder->initialize ( fileStream ) ) )
		return -4;

	AEAutoPtr<AEBaseAudioStream> audioStream;
	if ( FAILED ( AE_createAudioStream ( decoder, AETimeSpan ( 0, 0, 2, 000 ), &audioStream ) ) )
		return -5;

	if ( FAILED ( audioPlayer->setSourceStream ( audioStream ) ) )
		return -6;

	if ( FAILED ( audioPlayer->play () ) )
		return -7;

	while ( true )
	{
		AEPLAYERSTATE state;
		if ( FAILED ( audioPlayer->getState ( &state ) ) )
			return -8;

		if ( state != kAEPLAYERSTATE_PLAYING )
			break;

		AETimeSpan pos, duration;
		audioPlayer->getPosition ( &pos );
		audioPlayer->getDuration ( &duration );

		printf ( "%s/%s\n", ( ( std::string ) pos ).c_str (), ( ( std::string ) duration ).c_str () );

		Sleep ( 100 );
	}

	return 0;
}