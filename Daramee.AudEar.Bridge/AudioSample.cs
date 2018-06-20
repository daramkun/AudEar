using System;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar
{
	public class AudioSample : BaseObject
	{
		protected internal AudioSample ( IntPtr ptr )
			: base ( ptr )
		{ }

		public TimeSpan SampleTime
		{
			get
			{
				ErrorCode ec = AEBaseAudioSample_getSampleTime ( ObjectPtr, out TimeSpan time );
				Assert ( ec );
				return time;
			}
		}

		public TimeSpan SampleDuration
		{
			get
			{
				ErrorCode ec = AEBaseAudioSample_getSampleDuration ( ObjectPtr, out TimeSpan duration );
				Assert ( ec );
				return duration;
			}
		}

		public void Lock ( out IntPtr data, out long length )
		{
			ErrorCode ec = AEBaseAudioSample_lock ( ObjectPtr, out data, out length );
			Assert ( ec );
		}

		public void Unlock ()
		{
			ErrorCode ec = AEBaseAudioSample_unlock ( ObjectPtr );
			Assert ( ec );
		}
	}
}