using System;
using System.Collections.Generic;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar.Platforms
{
	public static class MediaFoundation
	{
		class MF
		{
			public int result;
			public MF () { result = MFStartup (); }
			~MF () { MFShutdown (); }
		}

		static readonly MF mf = new MF ();

		public static bool SupportMediaFoundation => mf.result == 0;
	}
}
