using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar
{
	public class AudEarBridgeStream : AudEarBaseStream
	{
		public AudEarBridgeStream ( Stream stream )
			: base ( GetCustomCallback ( stream ) )
		{ }

		private unsafe static IntPtr GetCustomCallback ( Stream stream )
		{
			AECustomStreamCallback callback = new AECustomStreamCallback
			{
				Data = GCHandle.Alloc ( stream ),
				Dispose = ( GCHandle s ) =>
				{
					//( s as Stream ).Dispose ();
					s.Free ();
					return ErrorCode.Success;
				},
				Read = ( GCHandle s, IntPtr buffer, long length, out long readed ) =>
				{
					try
					{
						byte [] bBuffer = new byte [ length ];
						readed = ( s.Target as Stream ).Read ( bBuffer, 0, ( int ) length );
						Marshal.Copy ( bBuffer, 0, buffer, ( int ) readed );
						return ErrorCode.Success;
					}
					catch { readed = 0; return ErrorCode.Fail; }
				},
				Write = ( GCHandle s, IntPtr buffer, long length, out long written ) =>
				{
					try
					{
						byte [] bBuffer = new byte [ length ];
						Marshal.Copy ( buffer, bBuffer, 0, ( int ) length );
						( s.Target as Stream ).Write ( bBuffer, 0, ( int ) length );
						written = length;
						return ErrorCode.Success;
					}
					catch { written = 0; return ErrorCode.Fail; }
				},
				Seek = ( GCHandle s, AESTREAMSEEK origin, long offset, out long seeked ) =>
				{
					try
					{
						seeked = ( s.Target as Stream ).Seek ( offset, ( SeekOrigin ) origin );
						return ErrorCode.Success;
					}
					catch { seeked = 0; return ErrorCode.Fail; }
				},
				Flush = ( GCHandle s ) =>
				{
					( s.Target as Stream ).Flush ();
					return ErrorCode.Success;
				},
				GetPosition = ( GCHandle s, out long pos ) =>
				{
					pos = ( s.Target as Stream ).Position;
					return ErrorCode.Success;
				},
				GetLength = ( GCHandle s, out long len ) =>
				{
					len = ( s.Target as Stream ).Length;
					return ErrorCode.Success;
				},
				CanSeek = ( GCHandle s, out bool can ) =>
				{
					can = ( s.Target as Stream ).CanSeek;
					return ErrorCode.Success;
				},
				CanRead = ( GCHandle s, out bool can ) =>
				{
					can = ( s.Target as Stream ).CanRead;
					return ErrorCode.Success;
				},
				CanWrite = ( GCHandle s, out bool can ) =>
				{
					can = ( s.Target as Stream ).CanWrite;
					return ErrorCode.Success;
				}
			};
			ErrorCode ec = AE_createCustomCallbackStream ( ref callback, out IntPtr st );
			Assert ( ec );
			return st;
		}
	}
}
