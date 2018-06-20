using System;
using System.Collections.Generic;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar
{
    public class AudioStream : AudEarBaseStream
    {
		public AudioStream ( AudioDecoder decoder, TimeSpan bufferSize )
			: base ( CreateAudioStream ( decoder, bufferSize ) )
		{ }

		private static IntPtr CreateAudioStream ( AudioDecoder decoder, TimeSpan bufferSize )
		{
			ErrorCode ec = AE_createAudioStream ( decoder.ObjectPtr, bufferSize, out IntPtr stream );
			Assert ( ec );
			return stream;
		}

		public WaveFormat WaveFormat
		{
			get
			{
				ErrorCode ec = AEBaseAudioStream_getWaveFormat ( ObjectPtr, out WaveFormat format );
				Assert ( ec );
				return format;
			}
		}

		public TimeSpan BufferSize
		{
			get
			{
				ErrorCode ec = AEBaseAudioStream_getBufferSize ( ObjectPtr, out TimeSpan bufferSize );
				Assert ( ec );
				return bufferSize;
			}
			set
			{
				ErrorCode ec = AEBaseAudioStream_setBufferSize ( ObjectPtr, value );
				Assert ( ec );
			}
		}

		public void Buffering ()
		{
			ErrorCode ec = AEBaseAudioStream_buffering ( ObjectPtr );
			Assert ( ec );
		}
	}
}
