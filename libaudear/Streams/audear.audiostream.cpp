#include "../audear.h"

#include "../InnerUtilities/MemoryStream.hpp"

#include <memory>

class __BufferedAudioStream
{
public:
	__BufferedAudioStream ( AEAUDIODECODER * decoder ) noexcept
		: _position ( 0 )
	{
		_decoder = decoder;
		AE_retainInterface ( _decoder );
		_decoder->getWaveFormat ( _decoder->object, &_format );

		_memStream = new __MemoryStream ();
	}
	~__BufferedAudioStream () noexcept
	{
		delete _memStream;
		_memStream = nullptr;
		AE_releaseInterface ( ( void ** ) &_decoder );
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		*format = _format;
		return AEERROR_NOERROR;
	}
	error_t buffering () noexcept
	{
		if ( ( _format.bytesPerSec / 100 ) > _memStream->getLength () )
		{
			while ( _format.bytesPerSec > _memStream->getLength () )
			{
				error_t err;
				AEAUDIOSAMPLE * as;
				err = _decoder->readSample ( _decoder->object, &as );
				if ( ISERROR ( err ) )
				{
					if ( err == AEERROR_END_OF_FILE )
						return AEERROR_END_OF_FILE;
					continue;
				}

				BYTE * readData;
				LONGLONG byteSize;
				if ( ISERROR ( as->lock ( as->object, &readData, &byteSize ) ) )
					return AEERROR_FAIL;

				_memStream->write ( readData, byteSize );

				as->unlock ( as->object );

				AE_releaseInterface ( ( void ** ) &as );
			}
		}
		return AEERROR_NOERROR;
	}

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		if ( _memStream->getLength () < len )
		{
			error_t err = buffering ();
			if ( ISERROR ( err ) && err != AEERROR_END_OF_FILE )
				return 0;
		}

		if ( _memStream->getLength () == 0 )
			return 0;

		auto retValue = _memStream->read ( buffer, len );
		_position += retValue;
		return retValue;
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		int64_t unit;
		switch ( origin )
		{
			case AESO_BEGIN: unit = offset; break;
			case AESO_CURRENT: unit = _position + offset; break;
			case AESO_END: unit = length () - offset; break;
		}

		if ( unit > length () )
			unit = length ();
		if ( unit < 0 )
			unit = 0;

		if ( ISERROR ( _decoder->setReadPosition ( _decoder->object, AETIMESPAN_initializeWithByteCount ( unit, _format.bytesPerSec ) ) ) )
			return -1;

		_memStream->setLength ( 0 );

		_position = offset;

		return 0;
	}
	int64_t tell () noexcept
	{
		return _position;
	}
	int64_t length () noexcept
	{
		AETIMESPAN timeSpan;
		if ( ISERROR ( _decoder->getDuration ( _decoder->object, &timeSpan ) ) )
			return -1;
		return ( int64_t ) ( AETIMESPAN_totalSeconds ( timeSpan ) * _format.bytesPerSec );
	}

private:
	AEAUDIODECODER * _decoder;
	AEWAVEFORMAT _format;

	__MemoryStream * _memStream;
	int64_t _position;
};

error_t AE_createBufferedAudioStream ( AEAUDIODECODER * decoder, AEAUDIOSTREAM ** ret )
{
	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );
	audioStream->object = new __BufferedAudioStream ( decoder );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __BufferedAudioStream* >( obj ); };
	audioStream->tag = "AudEar Buffered Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __BufferedAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __BufferedAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __BufferedAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __BufferedAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __BufferedAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __BufferedAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}

class __WholeAudioStream
{
public:
	__WholeAudioStream ( AEAUDIODECODER * decoder ) noexcept
		: _position ( 0 )
	{
		decoder->getWaveFormat ( decoder->object, &_format );

		AETIMESPAN duration;
		decoder->getDuration ( decoder->object, &duration );

		_length = ( int64_t ) ( AETIMESPAN_totalSeconds ( duration ) * _format.bytesPerSec );

		_buffer = AEAUDIOBUFFER<int8_t> ( _length );
		int64_t offset = 0;

		AEAUDIOSAMPLE * sample;
		while ( !ISERROR ( decoder->readSample ( decoder->object, &sample ) ) )
		{
			uint8_t * sampleBuffer;
			int64_t sampleLength;
			if ( !ISERROR ( sample->lock ( sample->object, &sampleBuffer, &sampleLength ) ) )
			{
				if ( offset + sampleLength < _length )
				{
					memcpy ( &_buffer [ offset ], sampleBuffer, ( size_t ) sampleLength );
					offset += sampleLength;
				}
				else
					printf ( "[WARNING] Buffer overflow\n" );
				sample->unlock ( sample->object );
			}
			AE_releaseInterface ( ( void ** ) &sample );
		}
	}
	~__WholeAudioStream () noexcept
	{ }

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		*format = _format;
		return AEERROR_NOERROR;
	}
	error_t buffering () noexcept { return AEERROR_END_OF_FILE; }

public:
	int64_t read ( uint8_t * buffer, int64_t len ) noexcept
	{
		auto retValue = ( _length < _position + len ) ? _length - _position : len;
		memcpy ( buffer, &_buffer [ 0 ] + _position, ( size_t ) retValue );
		_position += retValue;
		return retValue;
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		switch ( origin )
		{
			case AESO_BEGIN:
				if ( offset > _length || offset < 0 )
					return -1;
				_position = offset;
				break;

			case AESO_CURRENT:
				if ( _position + offset > _length || _position + offset < 0 )
					return -1;
				_position += offset;
				break;

			case AESO_END:
				if ( _length + offset > _length || _length + offset < 0 )
					return -1;
				_position = _length - offset;
				break;
		}
		return 0;
	}
	int64_t tell () noexcept { return _position; }
	int64_t length () noexcept { return _length; }

private:
	AEAUDIOBUFFER<int8_t> _buffer;
	AEWAVEFORMAT _format;
	int64_t _position;
	int64_t _length;
};

EXTC AEEXP error_t AE_createWholeAudioStream ( AEAUDIODECODER * decoder, AEAUDIOSTREAM ** ret )
{
	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );
	audioStream->object = new __WholeAudioStream ( decoder );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __WholeAudioStream* >( obj ); };
	audioStream->tag = "AudEar Whole Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __WholeAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __WholeAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __WholeAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __WholeAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __WholeAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __WholeAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
#include <atlbase.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <Wmcodecdsp.h>
#include <mediaobj.h>
#include <comdef.h>
#include <dmo.h>
#include <Dshow.h>

#pragma comment ( lib, "dmoguids.lib" )
#pragma comment ( lib, "wmcodecdspuuid.lib" )

class MediaBuffer : public IMediaBuffer
{
public:
	MediaBuffer ( int length )
		: maxLength ( ( length * 2 + 7 ) / 8 * 8 ), length ( length ), refCount ( 0 )
	{
		bytes = ( BYTE * ) CoTaskMemAlloc ( maxLength );
	}
	~MediaBuffer ()
	{
		CoTaskMemFree ( bytes );
	}

public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface ( REFIID riid, void ** ppvObject )
	{
		if ( riid == __uuidof ( IUnknown ) || riid == __uuidof( IMediaBuffer ) )
		{
			*ppvObject = this;
			AddRef ();
			return S_OK;
		}
		return E_FAIL;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef ( void ) { return InterlockedIncrement ( &refCount ); }
	virtual ULONG STDMETHODCALLTYPE Release ( void )
	{
		ULONG temp = InterlockedDecrement ( &refCount );
		if ( temp == 0 )
			delete this;
		return temp;
	}

public:
	virtual HRESULT STDMETHODCALLTYPE SetLength ( DWORD cbLength )
	{
		if ( cbLength <= ( DWORD ) maxLength )
		{
			length = cbLength;
			return S_OK;
		}
		else
		{
			maxLength = ( ( cbLength * 2 + 24 ) / 8 ) * 8;
			BYTE * temp = bytes;
			bytes = ( BYTE * ) CoTaskMemAlloc ( maxLength );
			memcpy ( bytes, temp, length );
			CoTaskMemFree ( temp );
			length = cbLength;
			return S_OK;
		}
	}

	virtual HRESULT STDMETHODCALLTYPE GetMaxLength ( _Out_  DWORD *pcbMaxLength )
	{
		*pcbMaxLength = maxLength;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetBufferAndLength (
		_Outptr_opt_result_bytebuffer_ ( *pcbLength )  BYTE **ppBuffer,
		_Out_opt_  DWORD *pcbLength )
	{
		if ( ppBuffer ) *ppBuffer = bytes;
		if ( pcbLength ) *pcbLength = length;
		return S_OK;
	}

private:
	ULONG refCount;
	BYTE * bytes;
	int length, maxLength;
};

class __DMOResamplerAudioStream
{
public:
	__DMOResamplerAudioStream ( AEAUDIOSTREAM * stream, WAVEFORMATEX * pwfx, IMediaObject * mediaObject )
		: _stream ( stream ), _wfx ( {} ), _mediaObject ( mediaObject )
	{
		AE_retainInterface ( stream );
		if ( pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
			_wfx = *( ( WAVEFORMATEXTENSIBLE* ) pwfx );
		else
			*( ( WAVEFORMATEX* ) &_wfx ) = *pwfx;

		CComQIPtr<IWMResamplerProps> resamplerProps = mediaObject;
		if ( resamplerProps )
			resamplerProps->SetHalfFilterLength ( 60 );

		stream->getWaveFormat ( stream->object, &_inputFormat );

		_inputBuffer = new MediaBuffer ( _inputFormat.bytesPerSec );
		_outputBuffer = new MediaBuffer ( _wfx.Format.nAvgBytesPerSec );
	}
	~__DMOResamplerAudioStream ()
	{
		AE_releaseInterface ( ( void ** ) &_stream );
	}

public:
	error_t getWaveFormat ( AEWAVEFORMAT * format ) noexcept
	{
		if ( format == nullptr ) return AEERROR_ARGUMENT_IS_NULL;
		*format = AEWAVEFORMAT_waveFormatFromWaveFormatEX ( ( WAVEFORMATEX* ) &_wfx );
		return AEERROR_NOERROR;
	}
	error_t buffering () noexcept
	{
		return _stream->buffering ( _stream->object );
	}

public:
	int64_t read ( uint8_t * buffer, int64_t length ) noexcept
	{
		int read = 0;
		bool cannotConvert = false;
		while ( read < length )
		{
			DWORD inputStatus;
			_mediaObject->GetInputStatus ( 0, &inputStatus );
			if ( inputStatus & DMO_INPUT_STATUSF_ACCEPT_DATA )
			{
				int64_t bytesToRead = outputToInput ( length - read );
				bytesToRead = min ( _wfx.Format.nAvgBytesPerSec * 2, bytesToRead );
				bytesToRead -= ( bytesToRead % _inputFormat.blockAlign );

				_inputBuffer->SetLength ( ( DWORD ) bytesToRead );
				BYTE * inputByteBuffer;
				DWORD inputByteBufferLength;
				_inputBuffer->GetBufferAndLength ( &inputByteBuffer, &inputByteBufferLength );

				if ( inputByteBufferLength == 0 )
					cannotConvert = true;

				int64_t bytesRead = _stream->read ( _stream->object, inputByteBuffer, inputByteBufferLength );

				if ( bytesRead <= 0 )
					break;

				_mediaObject->ProcessInput ( 0, _inputBuffer, 0, 0, 0 );

				do
				{
					_outputBuffer->SetLength ( 0 );

					DMO_OUTPUT_DATA_BUFFER outputDataBuffer = {};
					outputDataBuffer.pBuffer = _outputBuffer;
					DWORD status;
					_mediaObject->ProcessOutput ( 0, 1, &outputDataBuffer, &status );

					BYTE * outputByteBuffer;
					DWORD outputByteBufferLength;
					_outputBuffer->GetBufferAndLength ( &outputByteBuffer, &outputByteBufferLength );

					if ( outputByteBufferLength <= 0 )
						break;

					memcpy ( ( ( BYTE * ) buffer ) + read, outputByteBuffer, outputByteBufferLength );

					read += outputByteBufferLength;
				} while ( false );
			}
		}

		if ( read == 0 )
		{
			if ( cannotConvert )
				return -1;
		}

		return read;
	}
	int64_t seek ( int64_t offset, AESEEKORIGIN origin ) noexcept
	{
		return _stream->seek ( _stream->object, ( int64_t ) ( offset / ( double ) _wfx.Format.nAvgBytesPerSec * _inputFormat.bytesPerSec ), origin );
	}
	int64_t tell () noexcept
	{
		return ( int64_t ) ( _stream->tell ( _stream->object ) / ( double ) _inputFormat.bytesPerSec * _wfx.Format.nAvgBytesPerSec );
	}
	int64_t length () noexcept
	{
		return ( int64_t ) ( _stream->length ( _stream->object ) / ( double ) _inputFormat.bytesPerSec * _wfx.Format.nAvgBytesPerSec );
	}

private:
	int64_t outputToInput ( int64_t position )
	{
		int64_t result = ( int64_t ) ( position / ( _wfx.Format.nAvgBytesPerSec / ( double ) _inputFormat.bytesPerSec ) );
		result -= ( result % _inputFormat.blockAlign );
		return result;
	}

private:
	AEAUDIOSTREAM * _stream;
	WAVEFORMATEXTENSIBLE _wfx;
	AEWAVEFORMAT _inputFormat;
	
	CComPtr<IMediaObject> _mediaObject;

	CComPtr<MediaBuffer> _inputBuffer, _outputBuffer;
};

DMO_MEDIA_TYPE __convertMediaType ( WAVEFORMATEX * pwfx )
{
	DMO_MEDIA_TYPE mediaType = {};
	mediaType.majortype = MEDIATYPE_Audio;
	switch ( pwfx->wFormatTag )
	{
		case WAVE_FORMAT_PCM: mediaType.subtype = MEDIASUBTYPE_PCM; break;
		case WAVE_FORMAT_IEEE_FLOAT: mediaType.subtype = MEDIASUBTYPE_IEEE_FLOAT; break;
		case WAVE_FORMAT_EXTENSIBLE:
			{
				WAVEFORMATEXTENSIBLE * extensible = ( WAVEFORMATEXTENSIBLE* ) pwfx;
				if ( extensible->SubFormat == KSDATAFORMAT_SUBTYPE_PCM )
					mediaType.subtype = MEDIASUBTYPE_PCM;
				else if ( extensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT )
					mediaType.subtype = MEDIASUBTYPE_IEEE_FLOAT;
				else return {};
			}
			break;
		default:
			return {};
	}
	mediaType.bFixedSizeSamples = TRUE;
	mediaType.formattype = FORMAT_WaveFormatEx;
	mediaType.cbFormat = pwfx->cbSize;
	mediaType.pbFormat = ( BYTE* ) pwfx;
	return mediaType;
}

DMO_MEDIA_TYPE __convertMediaType ( const AEWAVEFORMAT & waveFormat )
{
	return __convertMediaType ( AEWAVEFORMAT_waveFormatExFromWaveFormat ( &waveFormat ) );
}

error_t AE_createDmoResamplerAudioStream ( AEAUDIOSTREAM * stream, WAVEFORMATEX * pwfx, AEAUDIOSTREAM ** ret )
{
	_com_ptr_t<_com_IIID<IMediaObject, &__uuidof ( IMediaObject )>> mediaObject;
	if ( FAILED ( mediaObject.CreateInstance ( __uuidof ( CResamplerMediaObject ) ) ) )
		return AEERROR_NOT_SUPPORTED_FEATURE;

	AEWAVEFORMAT inputFormat;
	stream->getWaveFormat ( stream->object, &inputFormat );

	DMO_MEDIA_TYPE inputType = __convertMediaType ( inputFormat ), outputType = __convertMediaType ( pwfx );

	if ( FAILED ( mediaObject->SetInputType ( 0, &inputType, 0 ) ) )
		return AEERROR_NOT_SUPPORTED_FORMAT;
	if ( FAILED ( mediaObject->SetOutputType ( 0, &outputType, 0 ) ) )
		return AEERROR_NOT_SUPPORTED_FORMAT;

	CoTaskMemFree ( inputType.pbFormat );

	AEAUDIOSTREAM * audioStream = AE_allocInterfaceType ( AEAUDIOSTREAM );

	audioStream->object = new __DMOResamplerAudioStream ( stream, pwfx, mediaObject );
	audioStream->free = [] ( void * obj ) { delete reinterpret_cast< __DMOResamplerAudioStream* >( obj ); };
	audioStream->tag = "AudEar DMO Resampler Audio Stream";
	audioStream->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __DMOResamplerAudioStream* >( obj )->getWaveFormat ( format ); };
	audioStream->buffering = [] ( void * obj ) { return reinterpret_cast< __DMOResamplerAudioStream* >( obj )->buffering (); };
	audioStream->read = [] ( void * obj, uint8_t * buffer, int64_t len ) { return reinterpret_cast< __DMOResamplerAudioStream* >( obj )->read ( buffer, len ); };
	audioStream->seek = [] ( void * obj, int64_t offset, AESEEKORIGIN origin ) { return reinterpret_cast< __DMOResamplerAudioStream* >( obj )->seek ( offset, origin ); };
	audioStream->tell = [] ( void * obj ) { return reinterpret_cast< __DMOResamplerAudioStream* >( obj )->tell (); };
	audioStream->length = [] ( void * obj ) { return reinterpret_cast< __DMOResamplerAudioStream* >( obj )->length (); };

	*ret = audioStream;

	return AEERROR_NOERROR;
}
#endif
