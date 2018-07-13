#include "../audear.h"

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP

#include <atlbase.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mftransform.h>

#include <VersionHelpers.h>

#pragma comment ( lib, "mf.lib" )
#pragma comment ( lib, "mfplat.lib" )
#pragma comment ( lib, "mfuuid.lib" )
#pragma comment ( lib, "mfreadwrite.lib" )

class __MFAudioSample
{
public:
	__MFAudioSample ( IMFSample * sample )
	{
		_sample = sample;
		_sample->ConvertToContiguousBuffer ( &_buffer );
	}
	~__MFAudioSample ()
	{ }

public:
	error_t getSampleTime ( AETIMESPAN * timeSpan )
	{
		if ( FAILED ( _sample->GetSampleTime ( ( LONGLONG * ) timeSpan ) ) )
			return AEERROR_FAIL;
		return AEERROR_NOERROR;
	}
	error_t getSampleDuration ( AETIMESPAN * timeSpan )
	{
		if ( FAILED ( _sample->GetSampleDuration ( ( LONGLONG * ) timeSpan ) ) )
			return AEERROR_FAIL;
		return AEERROR_NOERROR;
	}

public:
	error_t lock ( uint8_t ** buffer, int64_t * length )
	{
		DWORD temp;
		if ( FAILED ( _buffer->Lock ( buffer, nullptr, &temp ) ) )
			return AEERROR_FAIL;
		*length = temp;
		return AEERROR_NOERROR;
	}
	error_t unlock ()
	{
		if ( FAILED ( _buffer->Unlock () ) )
			return AEERROR_FAIL;
		return AEERROR_NOERROR;
	}

private:
	CComPtr<IMFSample> _sample;
	CComPtr<IMFMediaBuffer> _buffer;
};

class __MFAudioDecoder
{
public:
	__MFAudioDecoder ()
		: _sourceReader ( nullptr )
	{

	}
	~__MFAudioDecoder ()
	{
		MFShutdown ();
	}

public:
	error_t initialize ( AESTREAM * stream )
	{
		_sourceReader.Release ();

		CComPtr<IStream> comStream;
		if ( FAILED ( AE_convertWindowsComStream ( stream, &comStream ) ) )
			return AEERROR_FAIL;

		CComPtr<IMFByteStream> byteStream;
		if ( FAILED ( MFCreateMFByteStreamOnStream ( comStream, &byteStream ) ) )
			return AEERROR_FAIL;

		if ( FAILED ( MFCreateSourceReaderFromByteStream ( byteStream, nullptr, &_sourceReader ) ) )
			return AEERROR_FAIL;

		CComPtr<IMFMediaType> readingAudioMediaType;
		if ( FAILED ( MFCreateMediaType ( &readingAudioMediaType ) ) )
			return AEERROR_FAIL;
		if ( FAILED ( readingAudioMediaType->SetGUID ( MF_MT_MAJOR_TYPE, MFMediaType_Audio ) ) )
			return AEERROR_FAIL;
		if ( FAILED ( readingAudioMediaType->SetGUID ( MF_MT_SUBTYPE, MFAudioFormat_PCM ) ) )
			return AEERROR_FAIL;
		if ( FAILED ( readingAudioMediaType->SetUINT32 ( MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000 ) ) )
			return AEERROR_FAIL;

		if ( FAILED ( _sourceReader->SetCurrentMediaType ( MF_SOURCE_READER_FIRST_AUDIO_STREAM,
			nullptr, readingAudioMediaType ) ) )
			return AEERROR_FAIL;

		readingAudioMediaType.Release ();

		if ( FAILED ( _sourceReader->GetCurrentMediaType ( MF_SOURCE_READER_FIRST_AUDIO_STREAM, &readingAudioMediaType ) ) )
			return AEERROR_FAIL;

		WAVEFORMATEX * pwfx;
		UINT size;
		if ( FAILED ( MFCreateWaveFormatExFromMFMediaType ( readingAudioMediaType, &pwfx, &size ) ) )
			return AEERROR_FAIL;

		_waveFormat = AEWAVEFORMAT_waveFormatFromWaveFormatEX ( pwfx );

		CoTaskMemFree ( pwfx );

		return AEERROR_NOERROR;
	}

public:
	error_t getWaveFormat  ( AEWAVEFORMAT * format )
	{
		if ( _sourceReader == nullptr )
			return AEERROR_INVALID_CALL;
		*format = _waveFormat;
		return AEERROR_NOERROR;
	}
	error_t getDuration  ( AETIMESPAN * timeSpan )
	{
		if ( _sourceReader == nullptr )
			return AEERROR_INVALID_CALL;

		PROPVARIANT var;
		if ( FAILED ( _sourceReader->GetPresentationAttribute ( MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var ) ) )
			return AEERROR_FAIL;
		*timeSpan = AETIMESPAN_initializeWithTicks ( var.hVal.QuadPart );
		return AEERROR_NOERROR;
	}
	error_t getReadPosition  ( AETIMESPAN * timeSpan )
	{
		if ( _sourceReader == nullptr )
			return AEERROR_INVALID_CALL;
		*timeSpan = currentTime;
		return AEERROR_NOERROR;
	}
	error_t setReadPosition  ( AETIMESPAN timeSpan )
	{
		if ( _sourceReader == nullptr )
			return AEERROR_INVALID_CALL;
		PROPVARIANT pos;
		pos.vt = VT_I8;
		pos.hVal.QuadPart = AETIMESPAN_getTicks ( timeSpan );
		if ( FAILED ( _sourceReader->SetCurrentPosition ( GUID_NULL, pos ) ) )
			return AEERROR_FAIL;
		currentTime = timeSpan;
		return AEERROR_NOERROR;
	}

public:
	error_t readSample  ( AEAUDIOSAMPLE ** sample )
	{
		if ( _sourceReader == nullptr )
			return AEERROR_INVALID_CALL;

		CComPtr<IMFSample> mfSample;
		DWORD flags;
		if ( FAILED ( _sourceReader->ReadSample ( MF_SOURCE_READER_FIRST_AUDIO_STREAM,
			0, nullptr, &flags, nullptr, &mfSample ) ) )
			return AEERROR_FAIL;

		if ( flags & MF_SOURCE_READERF_ENDOFSTREAM )
			return AEERROR_END_OF_FILE;

		if ( mfSample == nullptr || FAILED ( mfSample->GetSampleTime ( ( LONGLONG * ) &currentTime ) ) )
			return AEERROR_FAIL;

		AEAUDIOSAMPLE * ret = AE_allocInterfaceType ( AEAUDIOSAMPLE );
		ret->object = new __MFAudioSample ( mfSample );
		ret->free = [] ( void * obj ) { delete reinterpret_cast< __MFAudioSample* >( obj ); };
		ret->tag = "AudEar Media Foundation Audio Decoder's Sample";
		ret->getSampleTime = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __MFAudioSample* >( obj )->getSampleTime ( timeSpan ); };
		ret->getSampleDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __MFAudioSample* >( obj )->getSampleDuration ( timeSpan ); };
		ret->lock = [] ( void * obj, uint8_t ** buffer, int64_t * len ) { return reinterpret_cast< __MFAudioSample* >( obj )->lock ( buffer, len ); };
		ret->unlock = [] ( void * obj ) { return reinterpret_cast< __MFAudioSample* >( obj )->unlock (); };

		*sample = ret;

		return AEERROR_NOERROR;
	}

private:
	CComPtr<IMFSourceReader> _sourceReader;
	AEWAVEFORMAT _waveFormat;

	AETIMESPAN currentTime;
};

error_t AE_createMediaFoundationAudioDecoder ( AEAUDIODECODER ** ret )
{
	if ( !IsWindowsVistaOrGreater () )
		return AEERROR_NOT_SUPPORTED_FEATURE;

	if ( FAILED ( MFStartup ( MF_VERSION ) ) )
		return AEERROR_NOT_SUPPORTED_FEATURE;

	AEAUDIODECODER * decoder = AE_allocInterfaceType ( AEAUDIODECODER );

	decoder->object = new __MFAudioDecoder ();
	decoder->free = [] ( void * obj ) { delete reinterpret_cast< __MFAudioDecoder* >( obj ); };
	decoder->tag = "AudEar Media Foundation Audio Decoder";
	decoder->initialize = [] ( void * obj, AESTREAM * stream ) { return reinterpret_cast< __MFAudioDecoder* >( obj )->initialize ( stream ); };
	decoder->getWaveFormat = [] ( void * obj, AEWAVEFORMAT * format ) { return reinterpret_cast< __MFAudioDecoder* >( obj )->getWaveFormat ( format ); };
	decoder->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __MFAudioDecoder* >( obj )->getDuration ( timeSpan ); };
	decoder->getReadPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __MFAudioDecoder* >( obj )->getReadPosition ( timeSpan ); };
	decoder->setReadPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __MFAudioDecoder* >( obj )->setReadPosition ( timeSpan ); };
	decoder->readSample = [] ( void * obj, AEAUDIOSAMPLE ** sample ) { return reinterpret_cast< __MFAudioDecoder* >( obj )->readSample ( sample ); };

	*ret = decoder;

	return AEERROR_NOERROR;
}

#endif