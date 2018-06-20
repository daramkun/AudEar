using Daramee.AudEar.Platforms;
using System;
using System.Runtime.InteropServices;

namespace Daramee.AudEar
{
	static class AudEarInterop
	{
		public enum AESTREAMSEEK : int
		{
			Set = 0,
			Cur = 1,
			End = 2,
		}

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern uint AERefObj_retain ( IntPtr obj );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern uint AERefObj_release ( IntPtr obj );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseStream_read ( IntPtr stream, [MarshalAs ( UnmanagedType.LPArray, SizeParamIndex = 0 )] byte [] buffer, long length, out long readed );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseStream_write ( IntPtr stream, [MarshalAs ( UnmanagedType.LPArray, SizeParamIndex = 0 )] byte [] data, long length, out long written );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseStream_seek ( IntPtr stream, AESTREAMSEEK offset, long count, out long seeked );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseStream_flush ( IntPtr stream );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseStream_getPosition ( IntPtr stream, [Out] out long pos );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseStream_getLength ( IntPtr stream, [Out] out long len );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseStream_canSeek ( IntPtr stream, [Out, MarshalAs ( UnmanagedType.I1 )] out bool can );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseStream_canRead ( IntPtr stream, [Out, MarshalAs ( UnmanagedType.I1 )] out bool can );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseStream_canWrite ( IntPtr stream, [Out, MarshalAs ( UnmanagedType.I1 )] out bool can );

		[StructLayout ( LayoutKind.Sequential )]
		public struct AECustomStreamCallback
		{
			public delegate ErrorCode DF ( GCHandle a );
			public delegate ErrorCode RW ( GCHandle a, IntPtr b, long c, [Out] out long d );
			public delegate ErrorCode S ( GCHandle a, AESTREAMSEEK b, long c, [Out] out long d );
			public delegate ErrorCode GPL ( GCHandle a, out long b );
			public delegate ErrorCode CSRW ( GCHandle a, [Out, MarshalAs ( UnmanagedType.I1 )] out bool b );
			
			public GCHandle Data;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public DF Dispose;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public RW Read;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public RW Write;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public S Seek;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public DF Flush;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public GPL GetPosition;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public GPL GetLength;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public CSRW CanSeek;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public CSRW CanRead;
			[MarshalAs ( UnmanagedType.FunctionPtr )]
			public CSRW CanWrite;
		}
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createCustomCallbackStream ( ref AECustomStreamCallback callback, [Out, MarshalAs ( UnmanagedType.SysInt )] out IntPtr stream );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioSample_getSampleTime ( IntPtr sample, [Out] out TimeSpan time );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioSample_getSampleDuration ( IntPtr sample, [Out] out TimeSpan time );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioSample_lock ( IntPtr sample, out IntPtr buffer, [Out] out long length );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioSample_unlock ( IntPtr sample );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioDecoder_initialize ( IntPtr decoder, IntPtr stream );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioDecoder_getWaveFormat ( IntPtr decoder, [Out] out WaveFormat format );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioDecoder_getDuration ( IntPtr decoder, [Out] out TimeSpan duration );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioDecoder_setReadPosition ( IntPtr decoder, TimeSpan time );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioDecoder_getSample ( IntPtr decoder, [Out] out IntPtr sample );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioStream_getBaseDecoder ( IntPtr stream, [Out] out IntPtr decoder );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioStream_getWaveFormat ( IntPtr stream, [Out] out WaveFormat format );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioStream_setBufferSize ( IntPtr stream, TimeSpan length );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioStream_getBufferSize ( IntPtr stream, [Out] out TimeSpan length );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioStream_buffering ( IntPtr stream );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_setSourceStream ( IntPtr player, IntPtr stream );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_play ( IntPtr player );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_pause ( IntPtr player );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_stop ( IntPtr player );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_getState ( IntPtr player, [Out] out PlayerState state );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_getDuration ( IntPtr player, [Out] out TimeSpan duration );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_getPosition ( IntPtr player, [Out] out TimeSpan time );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_setPosition ( IntPtr player, TimeSpan time );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_getVolume ( IntPtr player, [Out] out float volume );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AEBaseAudioPlayer_setVolume ( IntPtr player, float volume );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createAudioStream ( IntPtr decoder, TimeSpan bufferSize, [Out, MarshalAs ( UnmanagedType.SysInt )] out IntPtr stream );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createMediaFoundationDecoder ( [Out, MarshalAs ( UnmanagedType.SysInt )] out IntPtr decoder );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createXAudio2Player ( [MarshalAs ( UnmanagedType.IUnknown )] object xaudio2, [Out, MarshalAs ( UnmanagedType.SysInt )] out IntPtr player );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern ErrorCode AE_createWASAPIPlayer ( [MarshalAs ( UnmanagedType.IUnknown )] object mmDevice, AudioClientShareMode shareMode, [Out, MarshalAs ( UnmanagedType.SysInt )] out IntPtr player );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		[return: MarshalAs ( UnmanagedType.SysInt )]
		public static extern IntPtr AE_createMMapiInterface ();
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern void AE_destroyMMapiInterface (IntPtr api );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		[return: MarshalAs ( UnmanagedType.IUnknown )]
		public static extern object AE_getDefaultDevice (IntPtr api );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern IntPtr AE_getDeviceCount (IntPtr api );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		[return: MarshalAs ( UnmanagedType.IUnknown )]
		public static extern object AE_getDevice (IntPtr api, IntPtr index );

		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		[return: MarshalAs ( UnmanagedType.SysInt )]
		public static extern IntPtr AE_createXAudio2Interface ();
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern void AE_destroyXAudio2Interface (IntPtr xa );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		[return: MarshalAs ( UnmanagedType.IUnknown )]
		public static extern object AE_getIXAudio2 (IntPtr xa );
		[DllImport ( "libaudear.dll", CallingConvention = CallingConvention.StdCall )]
		public static extern IntPtr AE_getIXAudio2MasteringVoice ( IntPtr xa );

		[DllImport ( "mfplat.dll", CallingConvention = CallingConvention.StdCall )]
		[return: MarshalAs ( UnmanagedType.Error )]
		public static extern int MFStartup ( uint Version = ( 0x0002 << 16 ) | 0x0070, int dwFlags = 0 );
		[DllImport ( "mfplat.dll", CallingConvention = CallingConvention.StdCall )]
		[return: MarshalAs ( UnmanagedType.Error )]
		public static extern int MFShutdown ();
	}
}
