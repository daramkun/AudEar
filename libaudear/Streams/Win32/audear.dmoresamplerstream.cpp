#include "../../audear.h"
#include "audear.dmoresamplerstream.h"

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
		: maxLength ( ( length * 2 + 7 ) / 8 * 8 ), length ( length )
	{
		bytes = ( BYTE * ) CoTaskMemAlloc ( maxLength );
	}
	~MediaBuffer ()
	{
		CoTaskMemFree ( bytes );
	}

public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface (
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject )
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
		if ( cbLength <= maxLength )
		{
			length = cbLength;
			return S_OK;
		}
		else
		{
			maxLength = ( ( cbLength * 2 + 7 ) / 8 ) * 8;
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

DMO_MEDIA_TYPE __convert_media_type ( WAVEFORMATEX * pwfx )
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

class AEDMOResamplerAudioStream : public AEBaseAudioStream
{
public:
	AEDMOResamplerAudioStream () : inputFormat ( nullptr ), outputFormat ( nullptr ) { }
	~AEDMOResamplerAudioStream ()
	{
		if ( outputFormat ) CoTaskMemFree ( outputFormat );
		if ( inputFormat ) CoTaskMemFree ( inputFormat );
	}

public:
	error_t initialize ( AEBaseAudioStream * baseStream, WAVEFORMATEX * pwfx )
	{
		this->baseStream = baseStream;

		HRESULT hr;

		if ( FAILED ( hr = mediaObject.CreateInstance ( __uuidof ( CResamplerMediaObject ) ) ) )
			return AE_ERROR_FAIL;

		AEWaveFormat waveFormat;
		baseStream->getWaveFormat ( &waveFormat );

		inputFormat = waveFormat.getWaveFormatEx ();
		outputFormat = pwfx;

		DMO_MEDIA_TYPE inputType = __convert_media_type ( inputFormat ), outputType = __convert_media_type ( pwfx );

		if ( FAILED ( hr = mediaObject->SetInputType ( 0, &inputType, 0 ) ) )
			return hr;
		if ( FAILED ( hr = mediaObject->SetOutputType ( 0, &outputType, 0 ) ) )
			return hr;

		resamplerProps = mediaObject;
		if ( FAILED ( hr = resamplerProps->SetHalfFilterLength ( 60 ) ) )
			return hr;

		inputBuffer = new MediaBuffer ( inputFormat->nAvgBytesPerSec );
		outputBuffer = new MediaBuffer ( outputFormat->nAvgBytesPerSec );

		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t read ( OUT void * buffer, int64_t length, OUT int64_t * readed )
	{
		spinLock.lock ();

		int read = 0;
		while ( read < length )
		{
			DWORD inputStatus;
			mediaObject->GetInputStatus ( 0, &inputStatus );
			if ( inputStatus & DMO_INPUT_STATUSF_ACCEPT_DATA )
			{
				int64_t bytesToRead = outputToInput ( length - read );
				bytesToRead = min ( outputFormat->nAvgBytesPerSec * 2, bytesToRead );
				bytesToRead -= ( bytesToRead % inputFormat->nBlockAlign );

				inputBuffer->SetLength ( bytesToRead );
				BYTE * inputByteBuffer;
				DWORD inputByteBufferLength;
				inputBuffer->GetBufferAndLength ( &inputByteBuffer, &inputByteBufferLength );

				int64_t bytesRead;
				baseStream->read ( inputByteBuffer, inputByteBufferLength, &bytesRead );

				if ( bytesRead <= 0 )
					break;

				mediaObject->ProcessInput ( 0, inputBuffer, 0, 0, 0 );

				do
				{
					outputBuffer->SetLength ( 0 );

					DMO_OUTPUT_DATA_BUFFER outputDataBuffer = {};
					outputDataBuffer.pBuffer = outputBuffer;
					DWORD status;
					mediaObject->ProcessOutput ( 0, 1, &outputDataBuffer, &status );

					BYTE * outputByteBuffer;
					DWORD outputByteBufferLength;
					outputBuffer->GetBufferAndLength ( &outputByteBuffer, &outputByteBufferLength );

					if ( outputByteBufferLength <= 0 )
						break;

					memcpy ( ( ( BYTE * ) buffer ) + read, outputByteBuffer, outputByteBufferLength );

					read += outputByteBufferLength;
				} while ( false );
			}
		}

		if ( readed )
			*readed = read;

		spinLock.unlock ();

		return AE_ERROR_SUCCESS;
	}
	virtual error_t write ( IN const void * data, int64_t length, OUT int64_t * written ) { return E_NOTIMPL; }
	virtual error_t seek ( AESTREAMSEEK offset, int64_t count, OUT int64_t * seeked )
	{
		int64_t temp;
		error_t et = baseStream->seek ( offset, AETimeSpan::fromByteCount ( count, outputFormat->nAvgBytesPerSec ).getByteCount ( inputFormat->nAvgBytesPerSec ), &temp );
		if ( FAILED ( et ) )
			return et;
		if ( seeked )
		{
			*seeked = AETimeSpan::fromByteCount ( temp, inputFormat->nAvgBytesPerSec ).getByteCount ( outputFormat->nAvgBytesPerSec );
		}
		return AE_ERROR_SUCCESS;
	}
	virtual error_t flush () { return baseStream->flush (); }

public:
	virtual error_t getPosition ( OUT int64_t * pos )
	{
		error_t et;
		int64_t temp;
		if ( FAILED ( et = baseStream->getPosition ( &temp ) ) )
			return et;
		*pos = AETimeSpan::fromByteCount ( temp, inputFormat->nAvgBytesPerSec ).getByteCount ( outputFormat->nAvgBytesPerSec );
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getLength ( OUT int64_t * len )
	{
		error_t et;
		int64_t temp;
		if ( FAILED ( et = baseStream->getLength ( &temp ) ) )
			return et;
		*len = AETimeSpan::fromByteCount ( temp, inputFormat->nAvgBytesPerSec ).getByteCount ( outputFormat->nAvgBytesPerSec );
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t canSeek ( OUT bool * can ) { return baseStream->canSeek ( can ); }
	virtual error_t canRead ( OUT bool * can ) { return baseStream->canRead ( can ); }
	virtual error_t canWrite ( OUT bool * can ) { *can = false; return AE_ERROR_SUCCESS; }

public:
	virtual error_t getBaseDecoder ( OUT AEBaseAudioDecoder ** decoder ) { return baseStream->getBaseDecoder ( decoder ); }
	virtual error_t getWaveFormat ( OUT AEWaveFormat * format )
	{
		*format = AEWaveFormat ( outputFormat );
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t setBufferSize ( AETimeSpan length ) { return baseStream->setBufferSize ( length ); }
	virtual error_t getBufferSize ( OUT AETimeSpan * length ) { return baseStream->getBufferSize ( length ); }

public:
	virtual error_t buffering () { return baseStream->buffering (); }

private:
	int64_t outputToInput ( int64_t position )
	{
		int64_t result = ( int64_t ) ( position / ( outputFormat->nAvgBytesPerSec / ( double ) inputFormat->nAvgBytesPerSec ) );
		result -= ( result % inputFormat->nBlockAlign );
		return result;
	}

private:
	AEAutoPtr<AEBaseAudioStream> baseStream;

	_com_ptr_t<_com_IIID<IMediaObject, &__uuidof ( IMediaObject )>> mediaObject;
	CComQIPtr<IWMResamplerProps> resamplerProps;

	CComPtr<MediaBuffer> inputBuffer, outputBuffer;

	WAVEFORMATEX * inputFormat;
	WAVEFORMATEX * outputFormat;

	AESpinLock spinLock;
};

error_t AE_internal_createDMOResamplerStream ( AEBaseAudioStream * baseStream, WAVEFORMATEX * pwfx, AEBaseAudioStream ** stream )
{
	*stream = new AEDMOResamplerAudioStream ();
	error_t et = ( ( AEDMOResamplerAudioStream* ) *stream )->initialize ( baseStream, pwfx );
	if ( FAILED ( et ) )
	{
		( *stream )->release ();
		*stream = nullptr;
		return AE_ERROR_FAIL;
	}
	return AE_ERROR_SUCCESS;
}
