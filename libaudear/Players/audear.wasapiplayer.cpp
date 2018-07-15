#include "../audear.h"
#include "../InnerUtilities/Thread.hpp"
#include "../InnerUtilities/HighResolutionTimer.hpp"

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP

#include <atlbase.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <memory>

#define REFTIMES_PER_SEC									10000000
#define REFTIMES_PER_MILLISEC								10000

class __WASAPIAudioPlayer : public __Runnable
{
public:
	__WASAPIAudioPlayer ( IAudioClient * audioClient, WAVEFORMATEX * pwfx )
		: _audioClient ( audioClient ), _pwfx ( pwfx ), _bufferThread ( this )
		, _state ( AEPS_STOPPED ), _sourceStream ( nullptr )
		, _readedTime ( 0 ), _sampleTime ( { 0 } )
	{
		_performanceFrequency = __HRT_GetFrequency ();

		_hEvent = CreateEvent ( nullptr, false, false, nullptr );
		audioClient->SetEventHandle ( _hEvent );
		audioClient->GetBufferSize ( &_audioClientBufferSize );

		audioClient->GetService ( __uuidof( IAudioRenderClient ), ( void** ) &_audioRenderClient );
		audioClient->GetService ( __uuidof( ISimpleAudioVolume ), ( void** ) &_simpleAudioVolume );
	}
	~__WASAPIAudioPlayer ()
	{
		stop ();
		AE_releaseInterface ( ( void ** ) &_sourceStream );
		CoTaskMemFree ( _pwfx );
		CloseHandle ( _hEvent );
	}

public:
	error_t play ()
	{
		if ( _state == AEPS_PLAYING ) return AEERROR_NOERROR;

		if ( FAILED ( _audioClient->Start () ) )
			return AEERROR_FAIL;

		_bufferThread.Run ( nullptr );

		_state = AEPS_PLAYING;

		return AEERROR_NOERROR;
	}
	error_t pause ()
	{
		if ( _state == AEPS_PAUSED ) return AEERROR_NOERROR;

		_bufferThread.Terminate ( true );
		_bufferThread.Join ();

		_audioClient->Stop ();

		_state = AEPS_PAUSED;

		return AEERROR_NOERROR;
	}
	error_t stop ()
	{
		if ( _state == AEPS_STOPPED ) return AEERROR_NOERROR;

		_bufferThread.Terminate ( true );
		_bufferThread.Join ();

		_audioClient->Stop ();

		_sourceStream->seek ( _sourceStream->object, 0, AESO_BEGIN );

		_state = AEPS_STOPPED;

		return AEERROR_NOERROR;
	}

public:
	error_t state ( AEPLAYERSTATE * state )
	{
		*state = _state;
		return AEERROR_NOERROR;
	}

public:
	error_t getDuration ( AETIMESPAN * timeSpan )
	{
		if ( _sourceStream == nullptr ) return AEERROR_INVALID_CALL;
		int64_t temp = _sourceStream->length ( _sourceStream->object );
		*timeSpan = AETIMESPAN_initializeWithByteCount ( temp, _pwfx->nAvgBytesPerSec );
		return AEERROR_NOERROR;
	}
	error_t getPosition ( AETIMESPAN * timeSpan )
	{
		if ( _sourceStream == nullptr ) return AEERROR_INVALID_CALL;
		if ( _state == AEPS_STOPPED )
		{
			timeSpan->ticks = 0;
			return AEERROR_NOERROR;
		}
		else if ( _state == AEPS_PAUSED )
		{
			int64_t temp = _sourceStream->tell ( _sourceStream->object );
			*timeSpan = AETIMESPAN_initializeWithByteCount ( temp, _pwfx->nAvgBytesPerSec );
			return AEERROR_NOERROR;
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter ( &currentTime );

		timeSpan->ticks = _sampleTime.ticks + ( ( currentTime.QuadPart - _readedTime ) * 10000000 / _performanceFrequency );

		return AEERROR_NOERROR;
	}
	error_t setPosition ( AETIMESPAN timeSpan )
	{
		if ( _sourceStream == nullptr ) return AEERROR_INVALID_CALL;
		_sourceStream->seek ( _sourceStream->object, ( int64_t ) ( AETIMESPAN_totalSeconds ( timeSpan ) * _pwfx->nAvgBytesPerSec ), AESO_BEGIN );
		return AEERROR_NOERROR;
	}

public:
	error_t getVolume ( float * vol )
	{
		if ( FAILED ( _simpleAudioVolume->GetMasterVolume ( vol ) ) )
			return AEERROR_FAIL;
		return AEERROR_NOERROR;
	}
	error_t setVolume ( float vol )
	{
		if ( FAILED ( _simpleAudioVolume->SetMasterVolume ( vol, nullptr ) ) )
			return AEERROR_FAIL;
		return AEERROR_NOERROR;
	}

public:
	error_t setSource ( AEAUDIOSTREAM * audioStream )
	{
		if ( audioStream == nullptr )
			return AEERROR_ARGUMENT_IS_NULL;

		stop ();

		AE_releaseInterface ( ( void ** ) &_sourceStream );

		_sourceStream = audioStream;
		AE_retainInterface ( _sourceStream );

		AEWAVEFORMAT pwfxConv = AEWAVEFORMAT_waveFormatFromWaveFormatEX ( _pwfx ), streamWf;
		_sourceStream->getWaveFormat ( _sourceStream->object, &streamWf );
		if ( memcmp ( &pwfxConv, &streamWf, sizeof ( AEWAVEFORMAT ) ) != 0 )
		{
			AEAUDIOSTREAM * dmoResampler;
			if ( ISERROR ( AE_createDmoResamplerAudioStream ( _sourceStream, _pwfx, &dmoResampler ) ) )
				return AEERROR_FAIL;
			AE_releaseInterface ( ( void ** ) &_sourceStream );
			_sourceStream = dmoResampler;
		}

		return AEERROR_NOERROR;
	}

public:
	virtual void Run ( void * obj, const bool & terminate )
	{
		_readedTime = __HRT_GetCounter ();

		while ( !terminate )
		{
			if ( WaitForSingleObject ( _hEvent, 1000 ) == WAIT_TIMEOUT )
			{
				_state = AEPS_STOPPED;
				_readedTime = 0;
				_sampleTime.ticks = 0;
				return;
			}

			UINT paddingFrame;
			if ( FAILED ( _audioClient->GetCurrentPadding ( &paddingFrame ) ) )
				return;

			if ( _audioClientBufferSize == paddingFrame )
				continue;

			UINT32 availableFrames = _audioClientBufferSize - paddingFrame;
			int actualSize = availableFrames * _pwfx->nBlockAlign;

			if ( availableFrames <= 10 )
				continue;

			std::shared_ptr<BYTE []> readBuffer ( new BYTE [ actualSize ] );
			int64_t readed = _sourceStream->read ( _sourceStream->object, &readBuffer [ 0 ], actualSize );
			if ( readed == 0 )
			{
				_state = AEPS_STOPPED;
				_readedTime = 0;
				_sampleTime.ticks = 0;
				return;
			}
			if ( readed == -1 )
			{
				Sleep ( 1 );
				continue;
			}

			int64_t pos = _sourceStream->tell ( _sourceStream->object );
			_sampleTime = AETIMESPAN_initializeWithSeconds ( pos / ( double ) _pwfx->nAvgBytesPerSec );

			BYTE * data;
			if ( FAILED ( _audioRenderClient->GetBuffer ( availableFrames, &data ) ) )
				continue;

			memcpy ( data, &readBuffer [ 0 ], readed );

			_audioRenderClient->ReleaseBuffer ( ( UINT ) readed / _pwfx->nBlockAlign, 0 );

			_readedTime = __HRT_GetCounter ();

			Sleep ( 1 );
		}
	}

private:
	CComPtr<IAudioClient> _audioClient;
	CComPtr<IAudioRenderClient> _audioRenderClient;
	CComPtr<ISimpleAudioVolume> _simpleAudioVolume;

	HANDLE _hEvent;
	WAVEFORMATEX * _pwfx;
	UINT _audioClientBufferSize;

	AEPLAYERSTATE _state;
	int64_t _performanceFrequency;
	int64_t _readedTime;
	AETIMESPAN _sampleTime;

	AEAUDIOSTREAM * _sourceStream;

	__Thread _bufferThread;
};

IMMDevice * __getDefaultMMDevice ()
{
	HRESULT hr;
	CComPtr<IMMDeviceEnumerator> devEnum;
	if ( FAILED ( hr = CoCreateInstance ( __uuidof ( MMDeviceEnumerator ), nullptr, CLSCTX_ALL,
		__uuidof( IMMDeviceEnumerator ), ( void ** ) &devEnum ) ) )
		return nullptr;
	CComPtr<IMMDevice> dev;
	if ( FAILED ( devEnum->GetDefaultAudioEndpoint ( eRender, eConsole, &dev ) ) )
		return nullptr;
	return dev.Detach ();
}

error_t AE_createWASAPIAudioPlayer ( IUnknown * device, AEWASAPISHAREMODE shareMode, AEAUDIOPLAYER ** ret )
{
	CComQIPtr<IMMDevice> mmDevice = device;
	if ( mmDevice == nullptr )
	{
		mmDevice = __getDefaultMMDevice ();
		if ( mmDevice == nullptr )
			return AEERROR_ARGUMENT_IS_NULL;
	}

	CComPtr<IAudioClient> audioClient;
	if ( FAILED ( mmDevice->Activate ( __uuidof( IAudioClient ), CLSCTX_ALL, nullptr, ( void ** ) &audioClient ) ) )
		return AEERROR_NOT_SUPPORTED_FEATURE;

	WAVEFORMATEX * pwfx;
	if ( FAILED ( audioClient->GetMixFormat ( &pwfx ) ) )
		return AEERROR_FAIL;

	if ( pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
	{
		WAVEFORMATEXTENSIBLE * extensible = ( WAVEFORMATEXTENSIBLE * ) pwfx;
		extensible->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	}
	else if ( pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT )
	{
		pwfx->wFormatTag = WAVE_FORMAT_PCM;
	}

	if ( FAILED ( audioClient->Initialize ( ( AUDCLNT_SHAREMODE ) shareMode, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		REFTIMES_PER_SEC, 0, pwfx, nullptr ) ) )
		return AEERROR_NOT_SUPPORTED_FEATURE;

	AEAUDIOPLAYER * player = AE_allocInterfaceType ( AEAUDIOPLAYER );
	player->object = new __WASAPIAudioPlayer ( audioClient, pwfx );
	player->free = [] ( void * obj ) { delete reinterpret_cast< __WASAPIAudioPlayer* >( obj ); };
	player->tag = "AudEar WASAPI Audio Player";
	player->play = [] ( void * obj ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->play (); };
	player->pause = [] ( void * obj ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->pause (); };
	player->stop = [] ( void * obj ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->stop (); };
	player->getDuration = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->getDuration ( timeSpan ); };
	player->getPosition = [] ( void * obj, AETIMESPAN * timeSpan ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->getPosition ( timeSpan ); };
	player->setPosition = [] ( void * obj, AETIMESPAN timeSpan ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->setPosition ( timeSpan ); };
	player->state = [] ( void * obj, AEPLAYERSTATE * ps ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->state ( ps ); };
	player->setSource = [] ( void * obj, AEAUDIOSTREAM * stream ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->setSource ( stream ); };
	player->getVolume = [] ( void * obj, float * vol ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->getVolume ( vol ); };
	player->setVolume = [] ( void * obj, float vol ) { return reinterpret_cast< __WASAPIAudioPlayer* >( obj )->setVolume ( vol ); };

	*ret = player;

	return AEERROR_NOERROR;
}

error_t AE_createWASAPIAudioPlayerWithoutParameters ( AEAUDIOPLAYER ** ret )
{
	return AE_createWASAPIAudioPlayer ( nullptr, AEWASAPISM_SHARED, ret );
}
#endif