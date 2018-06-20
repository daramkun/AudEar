using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar
{
	public class BaseObject : IDisposable
	{
		protected internal IntPtr ObjectPtr { get; private set; }

		protected BaseObject ( IntPtr ptr )
		{
			ObjectPtr = ptr;
		}

		~BaseObject () { Dispose ( false ); }

		public void Dispose ()
		{
			Dispose ( true );
			GC.SuppressFinalize ( this );
		}

		protected virtual void Dispose ( bool disposing )
		{
			if ( ObjectPtr == IntPtr.Zero )
				return;

			AERefObj_release ( ObjectPtr );
			ObjectPtr = IntPtr.Zero;
		}

		protected static void Assert ( ErrorCode errorCode )
		{
			if ( errorCode == ErrorCode.Success ) return;
			switch ( errorCode )
			{
				case ErrorCode.EndOfFile: throw new IOException ();
				case ErrorCode.InvalidArgument: throw new ArgumentException ();
				case ErrorCode.InvalidCall: throw new InvalidOperationException ();
				case ErrorCode.NotImplemented: throw new NotImplementedException ();
				case ErrorCode.NotSupportedFeature: throw new NotSupportedException ();
				case ErrorCode.NotSupportedFormat: break;//throw new FormatException ();
				default: throw new Exception ();
			}
		}
	}
}
