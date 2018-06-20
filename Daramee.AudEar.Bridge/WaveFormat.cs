using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Daramee.AudEar
{
	public enum MediaFormat : int
	{
		Unknown = 0,
		PCM = 1,
		IEEE_Float = 2,
	}

	[StructLayout ( LayoutKind.Sequential )]
	public struct WaveFormat
	{
		public int Channels;
		public int BitsPerSample;
		public int SamplePerSec;
		public MediaFormat MediaFormat;

		public int BlockAlign => Channels * ( BitsPerSample / 8 );
		public int ByteRate => BlockAlign * SamplePerSec;
	}
}
