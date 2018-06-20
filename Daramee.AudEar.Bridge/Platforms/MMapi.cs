using System;
using System.Collections.Generic;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar.Platforms
{
	public enum AudioClientShareMode
	{
		Shared,
		Exclusive
	}

	public static class MMapi
	{
		class MM
		{
			public IntPtr mm;
			public MM () { mm = AE_createMMapiInterface (); }
			~MM () { AE_destroyMMapiInterface ( mm ); }
		}
		static readonly MM mm;

		static MMapi ()
		{
			mm = new MM ();
		}

		public static object GetDefaultDevice ()
		{
			return AE_getDefaultDevice ( mm.mm );
		}

		public static int Count => AE_getDeviceCount ( mm.mm ).ToInt32 ();
		public static object GetDevice ( int i ) => AE_getDevice ( mm.mm, new IntPtr ( i ) );
	}
}
