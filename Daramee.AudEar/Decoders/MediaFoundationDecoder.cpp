#include "../audear.h"

#if AEWINDOWS

#include <Windows.h>
#include <atlbase.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment ( lib, "mf.lib" )
#pragma comment ( lib, "mfplat.lib" )
#pragma comment ( lib, "mfuuid.lib" )
#pragma comment ( lib, "mfreadwrite.lib" )

class AEMediaFoundationDecodedSample : public AEDecodedSample
{
public:
	AEMediaFoundationDecodedSample ( IMFSample * sample )
		: _sample ( sample )
	{
		_sample->ConvertToContiguousBuffer ( &_buffer );
	}

public:
	virtual HRESULT __stdcall GetSampleTime ( AETimeSpan * time )
	{
		return _sample->GetSampleTime ( ( LONGLONG* ) time );
	}
	virtual HRESULT __stdcall GetSampleDuration ( AETimeSpan * duration )
	{
		return _sample->GetSampleDuration ( ( LONGLONG* ) duration );
	}

public:
	virtual HRESULT __stdcall Lock ( void ** buffer, LONGLONG * length )
	{
		HRESULT hr;

		DWORD currentLength;
		if ( FAILED ( hr = _buffer->Lock ( ( BYTE** ) buffer, nullptr, &currentLength ) ) )
			return hr;

		*length = currentLength;

		return S_OK;
	}
	virtual HRESULT __stdcall Unlock ()
	{
		return _buffer->Unlock ();
	}

private:
	CComPtr<IMFSample> _sample;
	CComPtr<IMFMediaBuffer> _buffer;
};

class AEMediaFoundationAudioDecoder : public AEAudioDecoder
{
public:
	AEMediaFoundationAudioDecoder ()
	{
		MFStartup ( MF_VERSION );
	}

	~AEMediaFoundationAudioDecoder ()
	{
		MFShutdown ();
	}

public:
	virtual HRESULT __stdcall Initialize ( IStream * stream, WAVEFORMATEX * outputFormat )
	{
		HRESULT hr;

		_sourceReader.Release ();

		_stream = stream;

		if ( FAILED ( hr = MFCreateMFByteStreamOnStream ( stream, &_byteStream ) ) )
			return hr;

		if ( FAILED ( hr = MFCreateSourceReaderFromByteStream ( _byteStream, nullptr, &_sourceReader ) ) )
			return hr;

		CComPtr<IMFMediaType> readingAudioMediaType;
		if ( FAILED ( hr = MFCreateMediaType ( &readingAudioMediaType ) ) )
			return hr;
		if ( FAILED ( hr = readingAudioMediaType->SetGUID ( MF_MT_MAJOR_TYPE, MFMediaType_Audio ) ) )
			return hr;
		if ( outputFormat != nullptr )
		{
			GUID audioFormat;
			switch ( outputFormat->wFormatTag )
			{
				case WAVE_FORMAT_PCM: audioFormat = MFAudioFormat_PCM; break;
				case WAVE_FORMAT_EXTENSIBLE:
					{
						WAVEFORMATEXTENSIBLE * extensible = ( WAVEFORMATEXTENSIBLE* ) outputFormat;
						if ( extensible->SubFormat == KSDATAFORMAT_SUBTYPE_PCM )
							audioFormat = MFAudioFormat_PCM;
						else if ( extensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT )
							audioFormat = MFAudioFormat_Float;
						else return E_INVALIDARG;
					}
					break;
				case WAVE_FORMAT_IEEE_FLOAT: audioFormat = MFAudioFormat_Float; break;
				default: return E_INVALIDARG;
			}
			if ( FAILED ( hr = readingAudioMediaType->SetGUID ( MF_MT_SUBTYPE, audioFormat ) ) )
				return hr;
			if ( FAILED ( hr = readingAudioMediaType->SetUINT32 ( MF_MT_AUDIO_NUM_CHANNELS, outputFormat->nChannels ) ) )
				return hr;
			if ( FAILED ( hr = readingAudioMediaType->SetUINT32 ( MF_MT_AUDIO_AVG_BYTES_PER_SECOND, outputFormat->nAvgBytesPerSec ) ) )
				return hr;
			if ( FAILED ( hr = readingAudioMediaType->SetUINT32 ( MF_MT_AUDIO_BITS_PER_SAMPLE, outputFormat->wBitsPerSample ) ) )
				return hr;
			if ( FAILED ( hr = readingAudioMediaType->SetUINT32 ( MF_MT_AUDIO_SAMPLES_PER_SECOND, outputFormat->nSamplesPerSec ) ) )
				return hr;
			if ( FAILED ( hr = readingAudioMediaType->SetUINT32 ( MF_MT_AUDIO_BLOCK_ALIGNMENT, outputFormat->nBlockAlign ) ) )
				return hr;
			if ( outputFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
			{
				WAVEFORMATEXTENSIBLE * extensible = ( WAVEFORMATEXTENSIBLE* ) outputFormat;
				if ( FAILED ( hr = readingAudioMediaType->SetUINT32 ( MF_MT_AUDIO_CHANNEL_MASK, extensible->dwChannelMask ) ) )
					return hr;
			}
		}
		else
		{
			if ( FAILED ( hr = readingAudioMediaType->SetGUID ( MF_MT_SUBTYPE, MFAudioFormat_PCM ) ) )
				return hr;
		}
		if ( FAILED ( hr = _sourceReader->SetCurrentMediaType ( MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, readingAudioMediaType ) ) )
			return hr;

		return S_OK;
	}

public:
	virtual HRESULT __stdcall GetDuration ( AETimeSpan * duration )
	{
		HRESULT hr;
		PROPVARIANT var;
		if ( FAILED ( hr = _sourceReader->GetPresentationAttribute ( MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var ) ) )
			return hr;
		*duration = AETimeSpan ( var.hVal.QuadPart );
		return S_OK;
	}
	virtual HRESULT __stdcall GetWaveFormat ( WAVEFORMATEX ** pwfx )
	{
		HRESULT hr;

		CComPtr<IMFMediaType> audioMediaType;
		if ( FAILED ( hr = _sourceReader->GetCurrentMediaType ( MF_SOURCE_READER_FIRST_AUDIO_STREAM, &audioMediaType ) ) )
			return hr;

		WAVEFORMATEX * temp;
		UINT size;
		if ( FAILED ( hr = MFCreateWaveFormatExFromMFMediaType ( audioMediaType, &temp, &size ) ) )
			return hr;

		*pwfx = ( WAVEFORMATEX* ) malloc ( size );
		memcpy ( *pwfx, temp, size );

		CoTaskMemFree ( temp );

		return S_OK;
	}

public:
	virtual HRESULT __stdcall SetReadPosition ( const AETimeSpan & pos, AETimeSpan * newPos )
	{
		HRESULT hr;

		PROPVARIANT position;
		position.vt = VT_I8;
		position.hVal.QuadPart = pos.GetTicks ();

		if ( FAILED ( hr = _sourceReader->SetCurrentPosition ( GUID_NULL, position ) ) )
			return hr;

		if ( newPos )
			*newPos = pos;
		return S_OK;
	}

public:
	virtual HRESULT __stdcall GetSample ( AEDecodedSample ** sample )
	{
		HRESULT hr;

		DWORD flags;
		CComPtr<IMFSample> s;
		if ( FAILED ( hr = _sourceReader->ReadSample ( MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &s ) ) )
			return hr;

		if ( flags & MF_SOURCE_READERF_ENDOFSTREAM )
			return E_EOF;

		*sample = new AEMediaFoundationDecodedSample ( s );

		return S_OK;
	}

private:
	CComPtr<IStream> _stream;
	CComPtr<IMFByteStream> _byteStream;
	CComPtr<IMFSourceReader> _sourceReader;
};

AEEXP HRESULT AE_CreateMediaFoundationAudioDecoder ( AEAudioDecoder ** decoder )
{
	*decoder = new AEMediaFoundationAudioDecoder ();
	return S_OK;
}

#endif