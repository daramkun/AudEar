#include "audear.h"

uint32_t AERefObj_retain ( AERefObj * obj ) { return obj->retain (); }
uint32_t AERefObj_release ( AERefObj * obj ) { return obj->release (); }

AEERROR AEBaseStream_read ( AEBaseStream * stream, void * buffer, int64_t length, int64_t * readed ) { return stream->read ( buffer, length, readed ); }
AEERROR AEBaseStream_write ( AEBaseStream * stream, const void * data, int64_t length, int64_t * written ) { return stream->write ( data, length, written ); }
AEERROR AEBaseStream_seek ( AEBaseStream * stream, AESTREAMSEEK offset, int64_t count, int64_t * seeked ) { return stream->seek ( offset, count, seeked ); }
AEERROR AEBaseStream_flush ( AEBaseStream * stream ) { return stream->flush (); }
AEERROR AEBaseStream_getPosition ( AEBaseStream * stream, int64_t * pos ) { return stream->getPosition ( pos ); }
AEERROR AEBaseStream_getLength ( AEBaseStream * stream, int64_t * len ) { return stream->getLength ( len ); }
AEERROR AEBaseStream_canSeek ( AEBaseStream * stream, bool * can ) { return stream->canSeek ( can ); }
AEERROR AEBaseStream_canRead ( AEBaseStream * stream, bool * can ) { return stream->canRead ( can ); }
AEERROR AEBaseStream_canWrite ( AEBaseStream * stream, bool * can ) { return stream->canWrite ( can ); }

AEERROR AE_createCustomCallbackStream ( const AECustomStreamCallback * callback, AEBaseStream ** stream )
{
	class AEInternalCustomCallbackStream : public AEBaseStream
	{
	public:
		AEInternalCustomCallbackStream ( const AECustomStreamCallback * callback )
			: callback ( *callback )
		{ }
		~AEInternalCustomCallbackStream ()
		{
			if ( callback.dispose ) callback.dispose ( callback.user_data );
		}

	public:
		virtual AEERROR read ( OUT void * buffer, int64_t length, OUT int64_t * readed )
		{
			if ( callback.read )
				return callback.read ( callback.user_data, buffer, length, readed );
			return AEERROR_NOT_IMPLEMENTED;
		}
		virtual AEERROR write ( IN const void * data, int64_t length, OUT int64_t * written )
		{
			if ( callback.write )
				return callback.write ( callback.user_data, data, length, written );
			return AEERROR_NOT_IMPLEMENTED;
		}
		virtual AEERROR seek ( AESTREAMSEEK offset, int64_t count, OUT int64_t * seeked )
		{
			if ( callback.seek )
				return callback.seek ( callback.user_data, offset, count, seeked );
			return AEERROR_NOT_IMPLEMENTED;
		}
		virtual AEERROR flush ()
		{
			if ( callback.flush )
				callback.flush ( callback.user_data );
			return AEERROR_NOT_IMPLEMENTED;
		}

	public:
		virtual AEERROR getPosition ( OUT int64_t * pos )
		{
			if ( callback.getPosition )
				return callback.getPosition ( callback.user_data, pos );
			return AEERROR_NOT_IMPLEMENTED;
		}
		virtual AEERROR getLength ( OUT int64_t * len )
		{
			if ( callback.getLength )
				return callback.getLength ( callback.user_data, len );
			return AEERROR_NOT_IMPLEMENTED;
		}

	public:
		virtual AEERROR canSeek ( OUT bool * can )
		{
			if ( callback.canSeek )
				return callback.canSeek ( callback.user_data, can );
			return AEERROR_NOT_IMPLEMENTED;
		}
		virtual AEERROR canRead ( OUT bool * can )
		{
			if ( callback.canRead )
				return callback.canRead ( callback.user_data, can );
			return AEERROR_NOT_IMPLEMENTED;
		}
		virtual AEERROR canWrite ( OUT bool * can )
		{
			if ( callback.canWrite )
				return callback.canWrite ( callback.user_data, can );
			return AEERROR_NOT_IMPLEMENTED;
		}

	private:
		AECustomStreamCallback callback;
	};

	if ( callback->user_data == nullptr ) return AEERROR_INVALID_ARGUMENT;
	if ( callback->read == nullptr && callback->write == nullptr )
		return AEERROR_INVALID_ARGUMENT;

	*stream = new AEInternalCustomCallbackStream ( callback );
	return AEERROR_SUCCESS;
}

AEERROR AEBaseAudioSample_getSampleTime ( AEBaseAudioSample * sample, AETimeSpan * time ) { return sample->getSampleTime ( time ); }
AEERROR AEBaseAudioSample_getSampleDuration ( AEBaseAudioSample * sample, AETimeSpan * time ) { return sample->getSampleDuration ( time ); }
AEERROR AEBaseAudioSample_lock ( AEBaseAudioSample * sample, void ** buffer, int64_t * length ) { return sample->lock ( buffer, length ); }
AEERROR AEBaseAudioSample_unlock ( AEBaseAudioSample * sample ) { return sample->unlock (); }

AEERROR AEBaseAudioDecoder_initialize ( AEBaseAudioDecoder * decoder, AEBaseStream * stream ) { return decoder->initialize ( stream ); }
AEERROR AEBaseAudioDecoder_getWaveFormat ( AEBaseAudioDecoder * decoder, AEWaveFormat * format ) { return decoder->getWaveFormat ( format ); }
AEERROR AEBaseAudioDecoder_getDuration ( AEBaseAudioDecoder * decoder, AETimeSpan * duration ) { return decoder->getDuration ( duration ); }
AEERROR AEBaseAudioDecoder_setReadPosition ( AEBaseAudioDecoder * decoder, AETimeSpan time ) { return decoder->setReadPosition ( time ); }
AEERROR AEBaseAudioDecoder_getSample ( AEBaseAudioDecoder * decoder, AEBaseAudioSample ** sample ) { return decoder->getSample ( sample ); }

AEERROR AEBaseAudioStream_getBaseDecoder ( AEBaseAudioStream * stream, AEBaseAudioDecoder ** decoder ) { return stream->getBaseDecoder ( decoder ); }
AEERROR AEBaseAudioStream_getWaveFormat ( AEBaseAudioStream * stream, AEWaveFormat * format ) { return stream->getWaveFormat ( format ); }
AEERROR AEBaseAudioStream_setBufferSize ( AEBaseAudioStream * stream, AETimeSpan length ) { return stream->setBufferSize ( length ); }
AEERROR AEBaseAudioStream_getBufferSize ( AEBaseAudioStream * stream, AETimeSpan * length ) { return stream->getBufferSize ( length ); }
AEERROR AEBaseAudioStream_buffering ( AEBaseAudioStream * stream ) { return stream->buffering (); }

AEERROR AEBaseAudioPlayer_setSourceStream ( AEBaseAudioPlayer * player, AEBaseAudioStream * stream ) { return player->setSourceStream ( stream ); }
AEERROR AEBaseAudioPlayer_play ( AEBaseAudioPlayer * player ) { return player->play (); }
AEERROR AEBaseAudioPlayer_pause ( AEBaseAudioPlayer * player ) { return player->pause (); }
AEERROR AEBaseAudioPlayer_stop ( AEBaseAudioPlayer * player ) { return player->stop (); }
AEERROR AEBaseAudioPlayer_getState ( AEBaseAudioPlayer * player, AEPLAYERSTATE * state ) { return player->getState ( state ); }
AEERROR AEBaseAudioPlayer_getDuration ( AEBaseAudioPlayer * player, AETimeSpan * duration ) { return player->getDuration ( duration ); }
AEERROR AEBaseAudioPlayer_getPosition ( AEBaseAudioPlayer * player, AETimeSpan * time ) { return player->getPosition ( time ); }
AEERROR AEBaseAudioPlayer_setPosition ( AEBaseAudioPlayer * player, AETimeSpan time ) { return player->setPosition ( time ); }
AEERROR AEBaseAudioPlayer_getVolume ( AEBaseAudioPlayer * player, float * volume ) { return player->getVolume ( volume ); }
AEERROR AEBaseAudioPlayer_setVolume ( AEBaseAudioPlayer * player, float volume ) { return player->setVolume ( volume ); }
