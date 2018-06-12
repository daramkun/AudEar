#ifndef __AUDEAR_FILESTREAM_H__
#define __AUDEAR_FILESTREAM_H__

EXTC error_t AEEXP AE_createFileStream ( const wchar_t * filename, bool readonly, OUT AEBaseStream ** stream );
EXTC error_t AEEXP AE_createMemoryStream ( int64_t capacity, OUT AEBaseStream ** stream );
inline error_t AE_createMemoryStream ( OUT AEBaseStream ** stream ) { return AE_createMemoryStream ( 0, stream ); }

#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
EXTC HRESULT AEEXP AE_convertStream ( AEBaseStream * stream, IStream ** istream );
#	endif

EXTC error_t AEEXP AE_createAudioStream ( AEBaseAudioDecoder * decoder, AETimeSpan bufferSize, AEBaseAudioStream ** stream );

#endif