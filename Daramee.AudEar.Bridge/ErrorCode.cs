using System;
using System.Collections.Generic;
using System.Text;

namespace Daramee.AudEar
{
	public enum ErrorCode : int
	{
		Success = 0,

		Unknown = -1,
		Fail = -2,
		InvalidArgument = -3,
		NotImplemented = -4,
		EndOfFile = -5,
		InvalidCall = -6,
		NotSupportedFormat = -7,
		NotSupportedFeature = -8,
	}
}
