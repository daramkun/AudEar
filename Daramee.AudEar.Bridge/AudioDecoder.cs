using System;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar
{
	public class AudioDecoder : BaseObject
	{
		protected AudioDecoder ( IntPtr ptr )
			: base ( ptr )
		{ }

		public void Initialize ( AudEarBaseStream stream )
		{
			ErrorCode ec = AEBaseAudioDecoder_initialize ( ObjectPtr, stream.ObjectPtr );
			Assert ( ec );
		}

		public WaveFormat WaveFormat
		{
			get
			{
				ErrorCode ec = AEBaseAudioDecoder_getWaveFormat ( ObjectPtr, out WaveFormat format );
				Assert ( ec );
				return format;
			}
		}

		public TimeSpan Duration
		{
			get
			{
				ErrorCode ec = AEBaseAudioDecoder_getDuration ( ObjectPtr, out TimeSpan duration );
				Assert ( ec );
				return duration;
			}
		}

		public TimeSpan ReadPosition
		{
			set
			{
				ErrorCode ec = AEBaseAudioDecoder_setReadPosition ( ObjectPtr, value );
				Assert ( ec );
			}
		}

		public AudioSample ReadSample ()
		{
			ErrorCode ec = AEBaseAudioDecoder_getSample ( ObjectPtr, out IntPtr sample );
			Assert ( ec );
			return new AudioSample ( sample );
		}
	}
}