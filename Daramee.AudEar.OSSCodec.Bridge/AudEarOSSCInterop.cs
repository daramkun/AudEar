using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Daramee.AudEar
{
	internal static class AudEarOSSCInterop
	{
		[DllImport ( "libaudear.osscodec.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createLameMp3Decoder ( out IntPtr decoder );
		[DllImport ( "libaudear.osscodec.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createOggOpusDecoder ( out IntPtr decoder );
		[DllImport ( "libaudear.osscodec.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createOggVorbisDecoder ( out IntPtr decoder );
		[DllImport ( "libaudear.osscodec.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createOggFLACDecoder ( out IntPtr decoder );
		[DllImport ( "libaudear.osscodec.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createFLACDecoder ( out IntPtr decoder );
		[DllImport ( "libaudear.osscodec.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createM4AAACDecoder ( out IntPtr decoder );
	}
}
