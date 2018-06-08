#include "../audear.h"

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment ( lib, "mf.lib" )
#pragma comment ( lib, "mfplat.lib" )
#pragma comment ( lib, "mfuuid.lib" )
#pragma comment ( lib, "mfreadwrite.lib" )

class AEMFAudioSample : public AEBaseAudioSample
{
public:
	AEMFAudioSample ( IMFSample * sample )
		: _sample ( sample )
	{
		_sample->ConvertToContiguousBuffer ( &_buffer );
	}

public:
	virtual error_t getSampleTime ( OUT AETimeSpan * time )
	{
		if ( FAILED ( _sample->GetSampleTime ( ( LONGLONG* ) time ) ) )
			return AE_ERROR_FAIL;
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getSampleDuration ( OUT AETimeSpan * time )
	{
		if ( FAILED ( _sample->GetSampleDuration ( ( LONGLONG* ) time ) ) )
			return AE_ERROR_FAIL;
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t lock ( OUT void ** buffer, OUT int64_t * length )
	{
		DWORD temp;
		if ( FAILED ( _buffer->Lock ( ( BYTE** ) buffer, nullptr, &temp ) ) )
			return AE_ERROR_FAIL;
		*length = temp;
		return AE_ERROR_SUCCESS;
	}
	virtual error_t unlock ()
	{
		if ( FAILED ( _buffer->Unlock () ) )
			return AE_ERROR_FAIL;
		return AE_ERROR_SUCCESS;
	}

private:
	CComPtr<IMFSample> _sample;
	CComPtr<IMFMediaBuffer> _buffer;
};

class AEMFAudioDecoder : public AEBaseAudioDecoder
{
public:
	virtual error_t initialize ( IN AEBaseStream * stream )
	{
		if ( stream == nullptr ) return AE_ERROR_INVALID_ARGUMENT;

		_sourceReader.Release ();
		_byteStream.Release ();
		_stream.Release ();

		HRESULT hr;
		error_t et;
		if ( FAILED ( hr = AE_convertStream ( stream, &_stream ) ) )
			return AE_ERROR_FAIL;
		if ( FAILED ( hr = MFCreateMFByteStreamOnStream ( _stream, &_byteStream ) ) )
			return AE_ERROR_FAIL;

		if ( FAILED ( hr = MFCreateSourceReaderFromByteStream ( _byteStream, nullptr, &_sourceReader ) ) )
			return AE_ERROR_FAIL;

		CComPtr<IMFMediaType> readingAudioMediaType;
		if ( FAILED ( hr = MFCreateMediaType ( &readingAudioMediaType ) ) )
			return AE_ERROR_FAIL;
		if ( FAILED ( hr = readingAudioMediaType->SetGUID ( MF_MT_MAJOR_TYPE, MFMediaType_Audio ) ) )
			return AE_ERROR_FAIL;
		if ( FAILED ( hr = readingAudioMediaType->SetGUID ( MF_MT_SUBTYPE, MFAudioFormat_PCM ) ) )
			return AE_ERROR_FAIL;

		if ( FAILED ( hr = _sourceReader->SetCurrentMediaType ( MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, readingAudioMediaType ) ) )
			return AE_ERROR_FAIL;

		readingAudioMediaType.Release ();

		if ( FAILED ( hr = _sourceReader->GetCurrentMediaType ( MF_SOURCE_READER_FIRST_AUDIO_STREAM, &readingAudioMediaType ) ) )
			return AE_ERROR_FAIL;

		WAVEFORMATEX * pwfx;
		UINT size;
		if ( FAILED ( hr = MFCreateWaveFormatExFromMFMediaType ( readingAudioMediaType, &pwfx, &size ) ) )
			return AE_ERROR_FAIL;

		_waveFormat = AEWaveFormat ( pwfx );

		CoTaskMemFree ( pwfx );

		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t getWaveFormat ( OUT AEWaveFormat * format )
	{
		if ( format == nullptr ) return AE_ERROR_INVALID_ARGUMENT;
		*format = _waveFormat;
		return AE_ERROR_SUCCESS;
	}
	virtual error_t getDuration ( OUT AETimeSpan * duration )
	{
		if ( duration == nullptr ) return AE_ERROR_INVALID_ARGUMENT;

		HRESULT hr;
		PROPVARIANT var;
		if ( FAILED ( hr = _sourceReader->GetPresentationAttribute ( MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var ) ) )
			return AE_ERROR_FAIL;
		*duration = AETimeSpan ( var.hVal.QuadPart );
		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t setReadPosition ( AETimeSpan time )
	{
		HRESULT hr;

		PROPVARIANT position;
		position.vt = VT_I8;
		position.hVal.QuadPart = time.getTicks ();

		if ( FAILED ( hr = _sourceReader->SetCurrentPosition ( GUID_NULL, position ) ) )
			return AE_ERROR_FAIL;

		return AE_ERROR_SUCCESS;
	}

public:
	virtual error_t getSample ( OUT AEBaseAudioSample ** sample )
	{
		HRESULT hr;

		DWORD flags;
		CComPtr<IMFSample> s;
		if ( FAILED ( hr = _sourceReader->ReadSample ( MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &s ) ) )
			return AE_ERROR_FAIL;

		if ( flags & MF_SOURCE_READERF_ENDOFSTREAM )
			return AE_ERROR_END_OF_FILE;

		*sample = new AEMFAudioSample ( s );

		return AE_ERROR_SUCCESS;
	}

private:
	CComPtr<IStream> _stream;
	CComPtr<IMFByteStream> _byteStream;
	CComPtr<IMFSourceReader> _sourceReader;

	AEWaveFormat _waveFormat;
};

EXTC error_t AEEXP AE_createMediaFoundationDecoder ( AEBaseAudioDecoder ** decoder )
{
	if ( decoder == nullptr ) return AE_ERROR_INVALID_ARGUMENT;
	*decoder = new AEMFAudioDecoder ();
	return AE_ERROR_SUCCESS;
}

#endif