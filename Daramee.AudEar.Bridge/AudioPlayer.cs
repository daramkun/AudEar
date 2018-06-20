using System;
using System.Collections.Generic;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar
{
	public class AudioPlayer : BaseObject
	{
		protected AudioPlayer ( IntPtr ptr )
			: base ( ptr )
		{ }

		public AudioStream SourceStream
		{
			set
			{
				ErrorCode ec = AEBaseAudioPlayer_setSourceStream ( ObjectPtr, value.ObjectPtr );
				Assert ( ec );
			}
		}

		public PlayerState PlayerState
		{
			get
			{
				ErrorCode ec = AEBaseAudioPlayer_getState ( ObjectPtr, out PlayerState state );
				Assert ( ec );
				return state;
			}
		}

		public TimeSpan Position
		{
			get
			{
				ErrorCode ec = AEBaseAudioPlayer_getPosition ( ObjectPtr, out TimeSpan time );
				Assert ( ec );
				return time;
			}
			set
			{
				ErrorCode ec = AEBaseAudioPlayer_setPosition ( ObjectPtr, value );
				Assert ( ec );
			}
		}

		public TimeSpan Duration
		{
			get
			{
				ErrorCode ec = AEBaseAudioPlayer_getDuration ( ObjectPtr, out TimeSpan duration );
				Assert ( ec );
				return duration;
			}
		}

		public float Volume
		{
			get
			{
				ErrorCode ec = AEBaseAudioPlayer_getVolume ( ObjectPtr, out float volume );
				Assert ( ec );
				return volume;
			}
			set
			{
				ErrorCode ec = AEBaseAudioPlayer_setVolume ( ObjectPtr, value );
				Assert ( ec );
			}
		}

		public void Play ()
		{
			ErrorCode ec = AEBaseAudioPlayer_play ( ObjectPtr );
			Assert ( ec );
		}

		public void Pause ()
		{
			ErrorCode ec = AEBaseAudioPlayer_pause ( ObjectPtr );
			Assert ( ec );
		}

		public void Stop ()
		{
			ErrorCode ec = AEBaseAudioPlayer_stop ( ObjectPtr );
			Assert ( ec );
		}
	}
}
