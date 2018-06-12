#include "../audear.h"

#if AE_PLATFORM_WINDOWS

#include <mmreg.h>

AEWaveFormat::AEWaveFormat ( WAVEFORMATEX * pwfx )
{
	channels = pwfx->nChannels;
	bitsPerSample = pwfx->wBitsPerSample;
	samplePerSec = pwfx->nSamplesPerSec;
	if ( pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
	{
		WAVEFORMATEXTENSIBLE * extensible = ( WAVEFORMATEXTENSIBLE* ) pwfx;
		if ( extensible->SubFormat == KSDATAFORMAT_SUBTYPE_PCM )
			format = AE_WAVEFORMAT_PCM;
		else if ( extensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT )
			format = AE_WAVEFORMAT_IEEE_FLOAT;
		else format = AE_WAVEFORMAT_UNKNOWN;
	}
	else
	{
		switch ( pwfx->wFormatTag )
		{
			case WAVE_FORMAT_PCM:
				format = AE_WAVEFORMAT_PCM;
				break;
			case WAVE_FORMAT_IEEE_FLOAT:
				format = AE_WAVEFORMAT_IEEE_FLOAT;
				break;
			default:
				format = AE_WAVEFORMAT_UNKNOWN;
				break;
		}
	}
}

WAVEFORMATEX * AEWaveFormat::getWaveFormatEx () const
{
	WAVEFORMATEX * pwfx = ( WAVEFORMATEX * ) CoTaskMemAlloc ( sizeof ( WAVEFORMATEX ) );
	pwfx->cbSize = sizeof ( WAVEFORMATEX );
	pwfx->nChannels = channels;
	pwfx->wBitsPerSample = bitsPerSample;
	pwfx->nSamplesPerSec = samplePerSec;
	pwfx->nBlockAlign = getBlockAlignment ();
	pwfx->nAvgBytesPerSec = getByteRate ();
	switch ( format )
	{
		case AE_WAVEFORMAT_PCM: pwfx->wFormatTag = WAVE_FORMAT_PCM; break;
		case AE_WAVEFORMAT_IEEE_FLOAT: pwfx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT; break;
		default: { CoTaskMemFree ( pwfx ); } return nullptr;
	}
	return pwfx;
}
#endif