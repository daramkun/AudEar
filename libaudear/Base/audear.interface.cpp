#include "../audear.h"

#include <atomic>

struct AEINTERFACE
{
	std::atomic<int32_t> refCount;
	void * object;
	void ( *dispose ) ( void * obj );
	const char * tag;
};

void * AE_allocInterface ( size_t cb )
{
	void * ret = AE_alloc ( cb );
	if ( ret == nullptr ) return nullptr;

	memset ( ret, 0, cb );

	reinterpret_cast< AEINTERFACE* >( ret )->refCount = 1;

	return ret;
}

int32_t AE_retainInterface ( void * obj )
{
	if ( obj == nullptr ) return 0;
	AEINTERFACE * i = reinterpret_cast< AEINTERFACE* >( obj );
	++i->refCount;
	return i->refCount;
}

int32_t AE_releaseInterface ( void ** obj )
{
	if ( obj == nullptr || *obj == nullptr ) return -1;
	auto aeInterface = reinterpret_cast< AEINTERFACE* >( *obj );
	int32_t ret = --aeInterface->refCount;
	if ( ret == 0 )
	{
		if ( aeInterface->dispose )
			aeInterface->dispose ( aeInterface->object );
		AE_free ( aeInterface );
	}
	*obj = nullptr;

	return ret;
}

EXTC AEEXP int64_t AESTREAM_read ( AESTREAM * stream, uint8_t * buffer, int64_t len )
{
	return stream->read ( stream->object, buffer, len );
}

EXTC AEEXP int64_t AESTREAM_seek ( AESTREAM * stream, int64_t offset, AESEEKORIGIN origin )
{
	return stream->seek ( stream->object, offset, origin );
}

EXTC AEEXP int64_t AESTREAM_tell ( AESTREAM * stream )
{
	return stream->tell ( stream->object );
}

EXTC AEEXP int64_t AESTREAM_length ( AESTREAM * stream )
{
	return stream->length ( stream->object );
}

EXTC AEEXP error_t AEAUDIOSAMPLE_getSampleTime ( AEAUDIOSAMPLE * sample, AETIMESPAN * timeSpan )
{
	return sample->getSampleTime ( sample->object, timeSpan );
}

EXTC AEEXP error_t AEAUDIOSAMPLE_getSampleDuration ( AEAUDIOSAMPLE * sample, AETIMESPAN * timeSpan )
{
	return sample->getSampleDuration ( sample->object, timeSpan );
}

EXTC AEEXP error_t AEAUDIOSAMPLE_lock ( AEAUDIOSAMPLE * sample, uint8_t ** buffer, int64_t * length )
{
	return sample->lock ( sample->object, buffer, length );
}

EXTC AEEXP error_t AEAUDIOSAMPLE_unlock ( AEAUDIOSAMPLE * sample )
{
	return sample->unlock ( sample->object );
}

EXTC AEEXP error_t AEAUDIODECODER_initialize ( AEAUDIODECODER * decoder, AESTREAM * stream )
{
	return decoder->initialize ( decoder->object, stream );
}

EXTC AEEXP error_t AEAUDIODECODER_getWaveFormat ( AEAUDIODECODER * decoder, AEWAVEFORMAT * format )
{
	return decoder->getWaveFormat ( decoder->object, format );
}

EXTC AEEXP error_t AEAUDIODECODER_getDuration ( AEAUDIODECODER * decoder, AETIMESPAN * timeSpan )
{
	return decoder->getDuration ( decoder->object, timeSpan );
}

EXTC AEEXP error_t AEAUDIODECODER_getReadPosition ( AEAUDIODECODER * decoder, AETIMESPAN * timeSpan )
{
	return decoder->getReadPosition ( decoder->object, timeSpan );
}

EXTC AEEXP error_t AEAUDIODECODER_setReadPosition ( AEAUDIODECODER * decoder, AETIMESPAN timeSpan )
{
	return decoder->setReadPosition ( decoder->object, timeSpan );
}

EXTC AEEXP error_t AEAUDIODECODER_readSample ( AEAUDIODECODER * decoder, AEAUDIOSAMPLE ** sample )
{
	return decoder->readSample ( decoder->object, sample );
}

EXTC AEEXP error_t AEAUDIOSTREAM_getWaveFormat ( AEAUDIOSTREAM * stream, AEWAVEFORMAT * format )
{
	return stream->getWaveFormat ( stream->object, format );
}

EXTC AEEXP error_t AEAUDIOSTREAM_buffering ( AEAUDIOSTREAM * stream )
{
	return stream->buffering ( stream->object );
}

EXTC AEEXP int64_t AEAUDIOSTREAM_read ( AEAUDIOSTREAM * stream, uint8_t * buffer, int64_t len )
{
	return stream->read ( stream->object, buffer, len );
}

EXTC AEEXP int64_t AEAUDIOSTREAM_seek ( AEAUDIOSTREAM * stream, int64_t offset, AESEEKORIGIN origin )
{
	return stream->seek ( stream->object, offset, origin );
}

EXTC AEEXP int64_t AEAUDIOSTREAM_tell ( AEAUDIOSTREAM * stream )
{
	return stream->tell ( stream->object );
}

EXTC AEEXP int64_t AEAUDIOSTREAM_length ( AEAUDIOSTREAM * stream )
{
	return stream->length ( stream->object );
}

EXTC AEEXP error_t AEAUDIOPLAYER_play ( AEAUDIOPLAYER * player )
{
	return player->play ( player->object );
}

EXTC AEEXP error_t AEAUDIOPLAYER_pause ( AEAUDIOPLAYER * player )
{
	return player->pause ( player->object );
}

EXTC AEEXP error_t AEAUDIOPLAYER_stop ( AEAUDIOPLAYER * player )
{
	return player->stop ( player->object );
}

EXTC AEEXP error_t AEAUDIOPLAYER_state ( AEAUDIOPLAYER * player, AEPLAYERSTATE * state )
{
	return player->state ( player->object, state );
}

EXTC AEEXP error_t AEAUDIOPLAYER_getDuration ( AEAUDIOPLAYER * player, AETIMESPAN * timeSpan )
{
	return player->getDuration ( player->object, timeSpan );
}

EXTC AEEXP error_t AEAUDIOPLAYER_getPosition ( AEAUDIOPLAYER * player, AETIMESPAN * timeSpan )
{
	return player->getPosition ( player->object, timeSpan );
}

EXTC AEEXP error_t AEAUDIOPLAYER_setPosition ( AEAUDIOPLAYER * player, AETIMESPAN timeSpan )
{
	return player->setPosition ( player->object, timeSpan );
}

EXTC AEEXP error_t AEAUDIOPLAYER_getVolume ( AEAUDIOPLAYER * player, float * vol )
{
	return player->getVolume ( player->object, vol );
}

EXTC AEEXP error_t AEAUDIOPLAYER_setVolume ( AEAUDIOPLAYER * player, float vol )
{
	return player->setVolume ( player->object, vol );
}

EXTC AEEXP error_t AEAUDIOPLAYER_setSource ( AEAUDIOPLAYER * player, AEAUDIOSTREAM * audioStream )
{
	return player->setSource ( player->object, audioStream );
}
