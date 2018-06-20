using System;
using System.Collections.Generic;
using System.Text;
using static Daramee.AudEar.AudEarInterop;

namespace Daramee.AudEar
{
	public class AudEarBaseStream : BaseObject
	{
		public bool CanSeek { get { ErrorCode ec = AEBaseStream_canSeek ( ObjectPtr, out bool can ); Assert ( ec ); return can; } }
		public bool CanRead { get { ErrorCode ec = AEBaseStream_canRead ( ObjectPtr, out bool can ); Assert ( ec ); return can; } }
		public bool CanWrite { get { ErrorCode ec = AEBaseStream_canWrite ( ObjectPtr, out bool can ); Assert ( ec ); return can; } }

		public long Position { get { ErrorCode ec = AEBaseStream_getPosition ( ObjectPtr, out long pos ); Assert ( ec ); return pos; } }
		public long Length { get { ErrorCode ec = AEBaseStream_getLength ( ObjectPtr, out long len ); Assert ( ec ); return len; } }

		protected AudEarBaseStream ( IntPtr ptr )
			: base ( ptr )
		{ }

		public long Read ( byte [] buffer, long length )
		{
			ErrorCode ec = AEBaseStream_read ( ObjectPtr, buffer, length, out long readed );
			Assert ( ec );
			return readed;
		}

		public long Write ( byte [] data, long length )
		{
			ErrorCode ec = AEBaseStream_write ( ObjectPtr, data, length, out long written );
			Assert ( ec );
			return written;
		}

		public long Seek ( System.IO.SeekOrigin origin, long offset )
		{
			ErrorCode ec = AEBaseStream_seek ( ObjectPtr, ( AESTREAMSEEK ) origin, offset, out long seeked );
			Assert ( ec );
			return seeked;
		}

		public void Flush ()
		{
			ErrorCode ec = AEBaseStream_flush ( ObjectPtr );
			Assert ( ec );
		}
	}
}
