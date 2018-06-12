#include <audear.h>
#include <audear.osscodec.h>
#pragma comment ( lib, "libaudear.lib" )
#pragma comment ( lib, "libaudear.osscodec.lib" )
#pragma comment ( lib, "XAudio2.lib" )
#pragma comment ( lib, "mf.lib" )
#pragma comment ( lib, "mfplat.lib" )

int main ( void )
{
//#define FILENAME TEXT ( "F:\\Multimedia\\Music\\KPOP&ROCK\\厩悼孤瘤记\\厩悼孤瘤记-04-给积变 么.mp3" )
//#define FILENAME TEXT ( "F:\\Multimedia\\Music\\Game\\DJMAX RESPECT\\DISK 1\\18. BlackCat.mp3" )
#define FILENAME TEXT ( "Test.mp3" )
//#define FILENAME TEXT ( "Test.ogg" )
//#define FILENAME TEXT ( "Test.opus" )
//#define FILENAME TEXT ( "Test.flac" )
//#define FILENAME TEXT ( "Test5_1ch.flac" )
//#define FILENAME TEXT ( "TestOggFlac.flac" )

	COM com;
	MediaFoundation mf;

	error_t et;
	
	/**/XAudio2 xaudio2;
	AEAutoPtr<AEBaseAudioPlayer> audioPlayer;
	if ( FAILED ( et = AE_createXAudio2Player ( xaudio2, &audioPlayer ) ) )
	return -1;/**/

	/*MMAPI mmapi;
	AEAutoPtr<AEBaseAudioPlayer> audioPlayer;
	if ( FAILED ( et = AE_createWASAPIPlayer ( mmapi, AUDCLNT_SHAREMODE_SHARED, &audioPlayer ) ) )
		return -1;/**/

	AEAutoPtr<AEBaseStream> fileStream;
	if ( FAILED ( et = AE_createFileStream ( FILENAME, true, &fileStream ) ) )
		return -2;

	AEAutoPtr<AEBaseAudioDecoder> decoder;
	//if ( FAILED ( et = AE_createMediaFoundationDecoder ( &decoder ) ) )
	//if ( FAILED ( et = AE_createOggVorbisDecoder ( &decoder ) ) )
	//if ( FAILED ( et = AE_createOggOpusDecoder ( &decoder ) ) )
	//if ( FAILED ( et = AE_createFLACDecoder ( &decoder ) ) )
	//if ( FAILED ( et = AE_createOggFLACDecoder ( &decoder ) ) )
	if ( FAILED ( et = AE_createLameMp3Decoder ( &decoder ) ) )
		return -3;
	if ( FAILED ( et = decoder->initialize ( fileStream ) ) )
		return -4;

	AEAutoPtr<AEBaseAudioStream> audioStream;
	if ( FAILED ( et = AE_createAudioStream ( decoder, AETimeSpan ( 0, 0, 1, 000 ), &audioStream ) ) )
		return -5;

	if ( FAILED ( et = audioPlayer->setSourceStream ( audioStream ) ) )
		return -6;

	if ( FAILED ( et = audioPlayer->play () ) )
		return -7;

	while ( true )
	{
		AEPLAYERSTATE state;
		if ( FAILED ( et = audioPlayer->getState ( &state ) ) )
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