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
//#define FILENAME TEXT ( "Test.m4a" )

	COM com;
	MediaFoundation mf;

	AEERROR et;
	
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
	//if ( FAILED ( et = AE_createM4AAACDecoder ( &decoder ) ) )
		return -3;
	if ( FAILED ( et = decoder->initialize ( fileStream ) ) )
		return -4;

	AEAutoPtr<AEBaseAudioStream> audioStream;
	if ( FAILED ( et = AE_createAudioStream ( decoder, AETimeSpan ( 0, 0, 1, 000 ), &audioStream ) ) )
		return -5;

	AEEqualizerBand bands [] =
	{
		{    32,     0, 1.f },
		{    64, -3.4f, 1.f },
		{   125,  1.9f, 1.f },
		{   250,  3.0f, 1.f },
		{   500,  6.0f, 1.f },
		{  1000,  3.0f, 1.f },
		{  2000,  1.7f, 1.f },
		{  4000, -2.2f, 1.f },
		{  8000, -4.6f, 1.f },
		{ 16000, -4.8f, 1.f },
	};

	AEAutoPtr<AEBaseAudioStream> equalizerStream;
	if ( FAILED ( et = AE_createEqualizationAudioStream ( audioStream, bands, sizeof ( bands ) / sizeof ( AEEqualizerBand ), &equalizerStream ) ) )
		return -6;

	if ( FAILED ( et = audioPlayer->setSourceStream ( equalizerStream ) ) )
		return -7;

	if ( FAILED ( et = audioPlayer->play () ) )
		return -8;

	while ( true )
	{
		AEPLAYERSTATE state;
		if ( FAILED ( et = audioPlayer->getState ( &state ) ) )
			return -9;

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