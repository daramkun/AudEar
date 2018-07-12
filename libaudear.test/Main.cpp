#include <crtdbg.h>

#include <audear.h>
#include <audear.filter.h>
#include <audear.osscodec.h>
#pragma comment ( lib, "libaudear.lib" )
#pragma comment ( lib, "libaudear.filter.lib" )
#pragma comment ( lib, "libaudear.osscodec.lib" )

#include <cstdio>

#define BANDWIDTH			0.8

int main ( void )
{
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

	AE_init ();
	AE_OSS_init ();

	//AE_unregisterAudioDecoderCreator ( AE_createMediaFoundationAudioDecoder );

#define FILENAME "./Samples/MP3 Sample with ID3 tag.mp3"
//#define FILENAME "./Samples/FLAC Sample.flac"
	AEAutoInterface<AESTREAM> fileStream;
	if ( ISERROR ( AE_createFileStream ( FILENAME, &fileStream ) ) )
		return -1;

	AEAutoInterface<AEAUDIODECODER> decoder;
	/*if ( ISERROR ( AE_createMediaFoundationAudioDecoder ( &decoder ) ) )
		return -2;
	
	if ( ISERROR ( decoder->initialize ( decoder->object, fileStream ) ) )
		return -3;*/
	if ( ISERROR ( AE_detectAudioDecoder ( fileStream, &decoder ) ) )
		return -2;
	printf ( "CODEC: %s\n", decoder->tag );

	AEWAVEFORMAT wf;
	decoder->getWaveFormat ( decoder->object, &wf );

	AEAutoInterface<AEAUDIOSTREAM> audioStream;
	if ( ISERROR ( AE_createBufferedAudioStream ( decoder, &audioStream ) ) )
		return -4;

	AEFILTERCOLLECTION filters = AE_initializeEqualizerFilterCollection ( wf.samplesPerSec,
		BANDWIDTH, AEEQP_NONE );
	//AEFILTERCOLLECTION filters = AE_initializeSingleItemFilterCollection ( AE_initializeLowPassFilter ( wf.samplesPerSec, 1200, 0.5 ) );
	
	AEAutoInterface<AEAUDIOSTREAM> toIEEE, toPCM, filterStream;
	if ( ISERROR ( AE_createPCMToIEEEFloatAudioStream ( audioStream, &toIEEE ) ) )
		return -6;
	if ( ISERROR ( AE_createFilterAudioStream ( toIEEE, &filters, false, &filterStream ) ) )
		return -7;
	if ( ISERROR ( AE_createIEEEFloatToPCMAudioStream ( filterStream, 16, &toPCM ) ) )
		return -8;

	AEAutoInterface<AEAUDIOPLAYER> player;
	if ( ISERROR ( AE_createWASAPIAudioPlayer ( nullptr, AEWASAPISM_SHARED, &player ) ) )
		return -9;

	if ( ISERROR ( player->setSource ( player->object, /*audioStream*/toPCM ) ) )
		return -10;

	//if ( ISERROR ( player->setPosition ( player->object, AETIMESPAN_initializeWithTimes ( 0, 3, 10, 0 ) ) ) )
	//	return -11;

	if ( ISERROR ( player->play ( player->object ) ) )
		return -12;

	int selected = 0;
	double equalizerValue [ 10 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	while ( true )
	{
		AEPLAYERSTATE playerState;
		player->state ( player->object, &playerState );

		if ( playerState != AEPS_PLAYING )
			break;

		AETIMESPAN pos, dur;
		player->getPosition ( player->object, &pos );
		player->getDuration ( player->object, &dur );

		printf ( "\r%02d:%02d:%02d.%03d/%02d:%02d:%02d.%03d",
			AETIMESPAN_getHours ( pos ), AETIMESPAN_getMinutes ( pos ), AETIMESPAN_getSeconds ( pos ), AETIMESPAN_getMillisecs ( pos ),
			AETIMESPAN_getHours ( dur ), AETIMESPAN_getMinutes ( dur ), AETIMESPAN_getSeconds ( dur ), AETIMESPAN_getMillisecs ( dur ) );
		
		printf ( "\t" );

		for ( int i = 0; i < 10; ++i )
		{
			HANDLE hConsole = GetStdHandle ( STD_OUTPUT_HANDLE );
			if ( selected == i )
				SetConsoleTextAttribute ( hConsole, BACKGROUND_BLUE | FOREGROUND_INTENSITY );
			//if ( selected == i )
			//	SetConsoleColors ( BACKGROUND_BLUE | FOREGROUND_INTENSITY );
			//else SetConsoleColors ( BACKGROUND_INTENSITY | FOREGROUND_INTENSITY );
			printf ( "%2.2lf", equalizerValue [ i ] );
			SetConsoleTextAttribute ( hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
			printf ( " " );
		}

		if ( GetAsyncKeyState ( VK_ESCAPE ) & 0x1 )
			break;

		if ( GetAsyncKeyState ( VK_UP ) & 0x1 )
		{
			equalizerValue [ selected ] += 0.1f;
			if ( equalizerValue [ selected ] > 30 ) equalizerValue [ selected ] = 30;
			filters = AE_initializeEqualizerFilterCollectionWithGainDB ( wf.samplesPerSec, BANDWIDTH, equalizerValue );
		}
		if ( GetAsyncKeyState ( VK_DOWN ) & 0x1 )
		{
			equalizerValue [ selected ] -= 0.1f;
			if ( equalizerValue [ selected ] < -30 ) equalizerValue [ selected ] = -30;
			filters = AE_initializeEqualizerFilterCollectionWithGainDB ( wf.samplesPerSec, BANDWIDTH, equalizerValue );
		}
		if ( GetAsyncKeyState ( VK_LEFT ) & 0x1 )
		{
			--selected;
			if ( selected <= 0 ) selected = 0;
		}
		if ( GetAsyncKeyState ( VK_RIGHT ) & 0x1 )
		{
			++selected;
			if ( selected > 10 ) selected = 9;
		}

		Sleep ( 1 );
	}

	player->stop ( player->object );

	return 0;
}