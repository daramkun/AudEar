using System;
using static Daramee.AudEar.AudEarOSSCInterop;

namespace Daramee.AudEar.Decoders
{
    public class LameMp3AudioDecoder : AudioDecoder
    {
		public LameMp3AudioDecoder ()
			: base ( CreateLameDecoder () )
		{ }

		private static IntPtr CreateLameDecoder ()
		{
			ErrorCode ec = AE_createLameMp3Decoder ( out IntPtr decoder );
			Assert ( ec );
			return decoder;
		}
	}
}
