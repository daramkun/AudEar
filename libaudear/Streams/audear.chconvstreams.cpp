#include "../audear.h"

#include <memory>

class __MultiChannelsToMonoAudioStream
{
public:
	__MultiChannelsToMonoAudioStream ( AEAUDIOSTREAM * stream, AEWAVEFORMAT wf )
		: _stream ( stream ), _wf ( wf )
	{ }
	~__MultiChannelsToMonoAudioStream ()
	{ }

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		*format = _wf;
		format->audioFormat = AEAF_IEEE_FLOAT;
		format->channels = 1;
		format->blockAlign = format->channels * ( format->bitsPerSample / 8 );
		format->bytesPerSec = format->blockAlign * format->samplesPerSec;
		return AEERROR_NOERROR;
	}
	error_t buffering () noexcept
	{
		return _stream->buffering ( _stream->object );
	}

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		int64_t readLen = len * _wf.channels;

		std::shared_ptr<float []> tempBuffer ( new float [ readLen / 4 ] );
		int64_t ret = _stream->read ( _stream->object, ( uint8_t* ) &tempBuffer [ 0 ], readLen );
		if ( ret == 0 )
			return 0;
		if ( ret <= -1 )
			return ret;

		int64_t readedLen = ret / 4;
		float * tempBuffer2 = ( float * ) buffer;
		for ( int i = 0, j = 0; i < readedLen; i += _wf.channels, ++j )
		{
			float temp = 0;
			for ( int ch = 0; ch < _wf.channels; ++ch )
				temp += tempBuffer [ i + ch ];
			tempBuffer2 [ j ] = temp / _wf.channels;
		}

		return ret / _wf.channels;
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, offset * _wf.channels, origin );
	}
	int64_t tell () noexcept
	{
		return _stream->tell ( _stream->object ) / _wf.channels;
	}
	int64_t length () noexcept
	{
		return _stream->length ( _stream->object ) / _wf.channels;
	}

public:
	AEAutoInterface<AEAUDIOSTREAM> _stream;
	AEWAVEFORMAT _wf;
};

error_t AE_createMultiChannelsToMonoAudioStream ( AEAUDIOSTREAM * stream, AEAUDIOSTREAM ** ret )
{
	AEWAVEFORMAT wf;
	stream->getWaveFormat ( stream->object, &wf );

	if ( wf.channels == 1 )
	{
		AE_retainInterface ( stream );
		*ret = stream;
		return AEERROR_NOERROR;
	}

	if ( wf.audioFormat != AEAF_IEEE_FLOAT )
	{
		if ( ISERROR ( AE_createPCMToIEEEFloatAudioStream ( stream, &stream ) ) )
			return AEERROR_NOT_SUPPORTED_FORMAT;
		stream->getWaveFormat ( stream->object, &wf );
	}

	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );

	audioStream->object = new __MultiChannelsToMonoAudioStream ( stream, wf );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __MultiChannelsToMonoAudioStream* >( obj ); };
	audioStream->tag = "AudEar Multi-channels to Mono-channel Converter Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __MultiChannelsToMonoAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __MultiChannelsToMonoAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __MultiChannelsToMonoAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __MultiChannelsToMonoAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __MultiChannelsToMonoAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __MultiChannelsToMonoAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}

class __MonoToStereoAudioStream
{
public:
	__MonoToStereoAudioStream ( AEAUDIOSTREAM * stream, AEWAVEFORMAT wf )
		: _stream ( stream ), _wf ( wf )
	{ }
	~__MonoToStereoAudioStream ()
	{ }

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		*format = _wf;
		format->audioFormat = AEAF_IEEE_FLOAT;
		format->channels = 2;
		format->blockAlign = format->channels * ( format->bitsPerSample / 8 );
		format->bytesPerSec = format->blockAlign * format->samplesPerSec;
		return AEERROR_NOERROR;
	}
	error_t buffering () noexcept
	{
		return _stream->buffering ( _stream->object );
	}

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		int64_t readLen = len / 2;

		std::shared_ptr<float []> tempBuffer ( new float [ readLen / 4 ] );
		int64_t ret = _stream->read ( _stream->object, ( uint8_t* ) &tempBuffer [ 0 ], readLen );
		if ( ret == 0 )
			return 0;
		if ( ret <= -1 )
			return ret;

		int64_t readedLen = ret / 4;
		float * tempBuffer2 = ( float * ) buffer;
		for ( int i = 0, j = 0; i < readedLen; ++i, j += 2 )
		{
			tempBuffer2 [ j ] = tempBuffer [ i ];
			tempBuffer2 [ j + 1 ] = tempBuffer [ i ];
		}

		return ret * 2;
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, offset / 2, origin );
	}
	int64_t tell () noexcept
	{
		return _stream->tell ( _stream->object ) * 2;
	}
	int64_t length () noexcept
	{
		return _stream->length ( _stream->object ) * 2;
	}

public:
	AEAutoInterface<AEAUDIOSTREAM> _stream;
	AEWAVEFORMAT _wf;
};

EXTC AEEXP error_t AE_createMonoToStereoAudioStream ( AEAUDIOSTREAM * stream, AEAUDIOSTREAM ** ret )
{
	AEWAVEFORMAT wf;
	stream->getWaveFormat ( stream->object, &wf );

	if ( wf.channels == 2 )
	{
		AE_retainInterface ( stream );
		*ret = stream;
		return AEERROR_NOERROR;
	}
	else if ( wf.channels > 2 )
		return AEERROR_NOT_SUPPORTED_FORMAT;

	if ( wf.audioFormat != AEAF_IEEE_FLOAT )
	{
		if ( ISERROR ( AE_createPCMToIEEEFloatAudioStream ( stream, &stream ) ) )
			return AEERROR_NOT_SUPPORTED_FORMAT;
		stream->getWaveFormat ( stream->object, &wf );
	}

	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );

	audioStream->object = new __MonoToStereoAudioStream ( stream, wf );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __MonoToStereoAudioStream* >( obj ); };
	audioStream->tag = "AudEar Mono-channels to Stereo-channel Converter Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __MonoToStereoAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __MonoToStereoAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __MonoToStereoAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __MultiChannelsToMonoAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __MonoToStereoAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __MonoToStereoAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}
