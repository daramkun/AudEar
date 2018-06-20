#ifndef __AUDEAR_FILESTREAM_H__
#define __AUDEAR_FILESTREAM_H__

EXTC AEEXP AEERROR STDCALL AE_createFileStream ( const wchar_t * filename, bool readonly, OUT AEBaseStream ** stream );
EXTC AEEXP AEERROR STDCALL AE_createMemoryStream ( int64_t capacity, OUT AEBaseStream ** stream );
inline AEERROR AE_createMemoryStream ( OUT AEBaseStream ** stream ) { return AE_createMemoryStream ( 0, stream ); }

#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
EXTC HRESULT AEEXP STDCALL AE_convertStream ( AEBaseStream * stream, IStream ** istream );
#	endif

EXTC AEEXP AEERROR STDCALL AE_createAudioStream ( AEBaseAudioDecoder * decoder, AETimeSpan bufferSize, AEBaseAudioStream ** stream );
EXTC AEEXP AEERROR STDCALL AE_createEqualizationAudioStream ( AEBaseAudioStream * baseStream, AEEqualizerBand * bands, uint32_t bandCount, AEBaseAudioStream ** stream );
EXTC AEEXP AEERROR STDCALL AE_updateEqualizationAudioStream ( AEBaseAudioStream * stream, AEEqualizerBand * bands, uint32_t bandCount );

#endif