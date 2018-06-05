#include "audear.ffmpeg.h"

#pragma warning ( disable: 4819 )
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#pragma warning ( default: 4819 )

#pragma comment ( lib, "libaudear.lib" )

#pragma comment ( lib, "avcodec.lib" )
#pragma comment ( lib, "avfilter.lib" )
#pragma comment ( lib, "avformat.lib" )
#pragma comment ( lib, "avutil.lib" )
#pragma comment ( lib, "swresample.lib" )
#pragma comment ( lib, "swscale.lib" )

class AEFFmpegAudioDecoder : public AEAudioDecoder
{
public:
	AEFFmpegAudioDecoder ()
	{

	}
	~AEFFmpegAudioDecoder ()
	{

	}

public:
	virtual HRESULT __stdcall Initialize ( IStream * stream, WAVEFORMATEX * outputFormat )
	{
		return E_NOTIMPL;
	}

public:
	virtual HRESULT __stdcall GetDuration ( AETimeSpan * duration )
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall GetWaveFormat ( WAVEFORMATEX ** pwfx )
	{
		return E_NOTIMPL;
	}

public:
	virtual HRESULT __stdcall SetReadPosition ( const AETimeSpan & pos, AETimeSpan * newPos )
	{
		return E_NOTIMPL;
	}

public:
	virtual HRESULT __stdcall GetSample ( AEDecodedSample ** sample )
	{
		return E_NOTIMPL;
	}

private:

};

HRESULT AE_CreateFFmpegAudioDecoder ( AEAudioDecoder ** decoder )
{
	*decoder = new AEFFmpegAudioDecoder ();
	return S_OK;
}
