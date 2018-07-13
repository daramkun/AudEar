#include "../audear.h"

#include <vector>
#include <map>
#include <mutex>

std::vector<error_t (*) ( AEAUDIODECODER ** )> g_detectorRegistered;
std::map<error_t (*) ( AEAUDIODECODER ** ), AEAutoInterface<AEAUDIODECODER>> g_createdDecoders;
std::mutex g_detectorMutex;

bool AE_registerAudioDecoderCreator ( error_t ( *creator ) ( AEAUDIODECODER ** ) )
{
	g_detectorMutex.lock ();
	auto found = std::find ( g_detectorRegistered.begin (), g_detectorRegistered.end (), creator );
	bool ret = ( found == g_detectorRegistered.end () );
	if ( ret )
		g_detectorRegistered.push_back ( creator );
	g_detectorMutex.unlock ();
	return ret;
}

void AE_unregisterAudioDecoderCreator ( error_t ( *creator ) ( AEAUDIODECODER** ) )
{
	g_detectorMutex.lock ();
	auto found = std::find ( g_detectorRegistered.begin (), g_detectorRegistered.end (), creator );
	if ( found != g_detectorRegistered.end () )
	{
		auto found2 = g_createdDecoders.find ( *found );
		if ( found2 != g_createdDecoders.end () )
			g_createdDecoders.erase ( found2 );
		g_detectorRegistered.erase ( found );
	}
	g_detectorMutex.unlock ();
}

error_t AE_detectAudioDecoder ( AESTREAM * stream, AEAUDIODECODER ** ret )
{
	error_t returnValue = AEERROR_NOT_SUPPORTED_FORMAT;

	g_detectorMutex.lock ();
	for ( auto i = g_detectorRegistered.begin (); i != g_detectorRegistered.end (); ++i )
	{
		stream->seek ( stream->object, 0, AESO_BEGIN );

		AEAUDIODECODER * decoder;

		auto found = g_createdDecoders.find ( *i );
		if ( found != g_createdDecoders.end () )
			decoder = found->second;
		else if ( ISERROR ( ( *i ) ( &decoder ) ) )
			continue;

		if ( ISERROR ( decoder->initialize ( decoder->object, stream ) ) )
			g_createdDecoders.insert ( std::pair<error_t (*) ( AEAUDIODECODER** ), AEAutoInterface<AEAUDIODECODER>> ( *i, decoder ) );
		else
		{
			*ret = decoder;
			if ( found != g_createdDecoders.end () )
				g_createdDecoders.erase ( found );
			returnValue = AEERROR_NOERROR;
			break;
		}
	}
	g_detectorMutex.unlock ();

	return returnValue;
}