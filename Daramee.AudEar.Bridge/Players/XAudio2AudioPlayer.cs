using Daramee.AudEar.Platforms;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar.Players
{
	public class XAudio2AudioPlayer : AudioPlayer
	{
		object xaudio2;

		public XAudio2AudioPlayer ( object ixaudio2 )
			: base ( CreateXAudio2Player ( ixaudio2 ) )
		{
			xaudio2 = ixaudio2;
		}
		public XAudio2AudioPlayer ()
			: this ( XAudio2.GetIXAudio2 () )
		{ }

		protected override void Dispose ( bool disposing )
		{
			if ( xaudio2 != null )
			{
				Marshal.ReleaseComObject ( xaudio2 );
				xaudio2 = null;
			}
			base.Dispose ( disposing );
		}

		private static IntPtr CreateXAudio2Player ( object ixaudio2 )
		{
			ErrorCode ec = AE_createXAudio2Player ( ixaudio2, out IntPtr player );
			Assert ( ec );
			return player;
		}
	}
}
