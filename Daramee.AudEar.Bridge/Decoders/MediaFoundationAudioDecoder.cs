using System;
using System.Collections.Generic;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar.Decoders
{
	public class MediaFoundationAudioDecoder : AudioDecoder
	{
		public MediaFoundationAudioDecoder ()
			: base ( CreateMFAudioDecoder () )
		{ }

		private static IntPtr CreateMFAudioDecoder ()
		{
			ErrorCode ec = AE_createMediaFoundationDecoder ( out IntPtr decoder );
			Assert ( ec );
			return decoder;
		}
	}
}
