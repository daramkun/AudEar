using System;
using System.Collections.Generic;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar.Platforms
{
    public static class XAudio2
    {
		class XA
		{
			public IntPtr xa;
			public XA () { xa = AE_createXAudio2Interface (); }
			~XA () { AE_destroyXAudio2Interface ( xa ); }
		}
		static readonly XA xa;

		static XAudio2 ()
		{
			xa = new XA ();
		}

		public static object GetIXAudio2 () { return AE_getIXAudio2 ( xa.xa ); }
		public static IntPtr GetIXAudio2MasteringVoice () { return AE_getIXAudio2MasteringVoice ( xa.xa ); }
    }
}
