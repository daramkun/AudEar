using Daramee.AudEar.Platforms;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar.Players
{
	public class WASAPIAudioPlayer : AudioPlayer
	{
		object mmdevice;

		public WASAPIAudioPlayer ( object mmdevice, AudioClientShareMode shareMode )
			: base ( CreateWASAPIPlayer ( mmdevice, shareMode ) )
		{
			this.mmdevice = mmdevice;
		}
		public WASAPIAudioPlayer ( AudioClientShareMode shareMode = AudioClientShareMode.Shared )
			: this ( MMapi.GetDefaultDevice (), shareMode )
		{ }

		protected override void Dispose ( bool disposing )
		{
			if ( mmdevice != null )
			{
				Marshal.ReleaseComObject ( mmdevice );
				mmdevice = null;
			}
			base.Dispose ( disposing );
		}

		private static IntPtr CreateWASAPIPlayer ( object mmdevice, AudioClientShareMode shareMode )
		{
			ErrorCode ec = AE_createWASAPIPlayer ( mmdevice, shareMode, out IntPtr player );
			Assert ( ec );
			return player;
		}
	}
}
