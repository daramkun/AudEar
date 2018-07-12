#include "../audear.h"

#if AE_PLATFORM_WINDOWS

#include <mmreg.h>

AEWAVEFORMAT AEWAVEFORMAT_waveFormatFromWaveFormatEX ( const WAVEFORMATEX * pwfx )
{
	AEWAVEFORMAT wf = {};
	wf.channels = pwfx->nChannels;
	wf.bitsPerSample = pwfx->wBitsPerSample;
	wf.samplesPerSec = pwfx->nSamplesPerSec;
	wf.blockAlign = pwfx->nBlockAlign;
	wf.bytesPerSec = pwfx->nAvgBytesPerSec;
	if ( pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
	{
		WAVEFORMATEXTENSIBLE * extensible = ( WAVEFORMATEXTENSIBLE* ) pwfx;
		if ( extensible->SubFormat == KSDATAFORMAT_SUBTYPE_PCM )
			wf.audioFormat = AEAF_PCM;
		else if ( extensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT )
			wf.audioFormat = AEAF_IEEE_FLOAT;
		else
			wf.audioFormat = AEAF_UNKNOWN;
	}
	else
	{
		switch ( pwfx->wFormatTag )
		{
			case WAVE_FORMAT_PCM:
				wf.audioFormat = AEAF_PCM;
				break;
			case WAVE_FORMAT_IEEE_FLOAT:
				wf.audioFormat = AEAF_IEEE_FLOAT;
				break;
			default:
				wf.audioFormat = AEAF_UNKNOWN;
				break;
		}
	}
	return wf;
}

WAVEFORMATEX * AEWAVEFORMAT_waveFormatExFromWaveFormat ( const AEWAVEFORMAT * waveFormat )
{
	WAVEFORMATEX * pwfx = ( WAVEFORMATEX* ) CoTaskMemAlloc ( sizeof ( WAVEFORMATEX ) );
	pwfx->cbSize = sizeof ( WAVEFORMATEX );
	pwfx->nChannels = waveFormat->channels;
	pwfx->wBitsPerSample = waveFormat->bitsPerSample;
	pwfx->nSamplesPerSec = waveFormat->samplesPerSec;
	pwfx->nBlockAlign = waveFormat->blockAlign;
	pwfx->nAvgBytesPerSec = waveFormat->bytesPerSec;
	switch ( waveFormat->audioFormat )
	{
		case AEAF_UNKNOWN: pwfx->wFormatTag = WAVE_FORMAT_UNKNOWN; break;
		case AEAF_PCM: pwfx->wFormatTag = WAVE_FORMAT_PCM; break;
		case AEAF_IEEE_FLOAT: pwfx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT; break;
	}
	return pwfx;
}

#endif
