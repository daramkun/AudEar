#include "../audear.h"

#include <memory>
#include <cmath>

class __FilterAudioStream
{
public:
	__FilterAudioStream ( AEAUDIOSTREAM * stream, AEWAVEFORMAT wf, AEFILTERCOLLECTION * collection, bool collectionConst )
		: _stream ( stream ), _wf ( wf ), collectionConst ( collectionConst )
	{
		AE_retainInterface ( stream );

		if ( collectionConst )
		{
			this->collection = new AEFILTERCOLLECTION;
			memcpy ( this->collection, collection, sizeof ( AEFILTERCOLLECTION ) );
		}
		else
			this->collection = collection;

	}
	~__FilterAudioStream ()
	{
		AE_releaseInterface ( ( void ** ) &_stream );
		if ( collectionConst )
			delete collection;
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		*format = _wf;
		return AEERROR_NOERROR;
	}
	error_t buffering () noexcept
	{
		return _stream->buffering ( _stream->object );
	}

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		int64_t ret = _stream->read ( _stream->object, buffer, len );
		if ( ret <= 0 ) return 0;
		if ( ret % _wf.blockAlign != 0 ) return -1;

		int64_t length = ret / 4;
		float * tempBuffer = ( float * ) buffer;
		for ( int i = 0; i < length; ++i )
		{
			int ch = i % _wf.channels;
			for ( int f = 0; f < collection->filters_size; ++f )
				tempBuffer [ i ] = AEBIQUADFILTER_process ( &collection->filters [ f ], tempBuffer [ i ], ch );
		}

		return ret;
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, offset, origin );
	}
	int64_t tell () noexcept
	{
		return _stream->tell ( _stream->object );
	}
	int64_t length () noexcept
	{
		return _stream->length ( _stream->object );
	}

private:
	AEAUDIOSTREAM * _stream;
	AEWAVEFORMAT _wf;

	AEFILTERCOLLECTION * collection;
	bool collectionConst;
};

error_t AE_createFilterAudioStream ( AEAUDIOSTREAM * stream, AEFILTERCOLLECTION * collection, bool collectionConst, AEAUDIOSTREAM ** ret )
{
	if ( stream == nullptr || ret == nullptr ) return AEERROR_ARGUMENT_IS_NULL;

	AEWAVEFORMAT wf;
	stream->getWaveFormat ( stream->object, &wf );
	if ( wf.audioFormat != AEAF_IEEE_FLOAT )
		return AEERROR_NOT_SUPPORTED_FORMAT;

	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );

	audioStream->object = new __FilterAudioStream ( stream, wf, collection, collectionConst );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __FilterAudioStream* >( obj ); };
	audioStream->tag = "AudEar Filtering Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __FilterAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __FilterAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __FilterAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __FilterAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __FilterAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __FilterAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}