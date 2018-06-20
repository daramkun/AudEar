using Daramee.AudEar.Decoders;
using Daramee.AudEar.Players;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Daramee.AudEar.Bridge.Test
{
	class Program
	{
		[MTAThread]
		static void Main ( string [] args )
		{
			AppDomain.CurrentDomain.UnhandledException += ( object sender, UnhandledExceptionEventArgs e ) =>
			{
				Console.WriteLine ( e.ExceptionObject );
			};

			using ( Stream stream = new FileStream ( @"E:\Projects\GitHub\AudEar\libaudear.test\Test.ogg", FileMode.Open, FileAccess.Read ) )
			{
				using ( AudEarBaseStream aes = new AudEarBridgeStream ( stream ) )
				{
					using ( AudioDecoder decoder = new OggVorbisAudioDecoder () )
					{
						decoder.Initialize ( aes );
						using ( AudioStream audioStream = new AudioStream ( decoder, TimeSpan.FromSeconds ( 1 ) ) )
						{
							using ( AudioPlayer player = new WASAPIAudioPlayer () )
							{
								player.SourceStream = audioStream;
								player.Play ();

								while ( player.PlayerState == PlayerState.Playing )
								{
									Console.WriteLine ( $"{player.Position}/{player.Duration}" );
								}
							}
						}
					}
				}
			}
		}
	}
}
