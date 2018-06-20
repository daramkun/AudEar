using System;
using static Daramee.AudEar.AudEarOSSCInterop;

namespace Daramee.AudEar.Decoders
{
	public class OggVorbisAudioDecoder : AudioDecoder
	{
		public OggVorbisAudioDecoder ()
			: base ( CreateVorbisDecoder () )
		{ }

		private static IntPtr CreateVorbisDecoder ()
		{
			ErrorCode ec = AE_createOggVorbisDecoder ( out IntPtr decoder );
			Assert ( ec );
			return decoder;
		}
	}
}
