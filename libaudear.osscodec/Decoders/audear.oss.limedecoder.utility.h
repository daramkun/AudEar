#ifndef __AUDEAR_LIMEDECODER_UTILITY_H__
#define __AUDEAR_LIMEDECODER_UTILITY_H__

// Get ID3 v2.x Tag Length
inline int get_id3v2_length ( uint8_t * buffer, int maxSize )
{
	if ( !( buffer [ 0 ] == 'I' && buffer [ 1 ] == 'D' && buffer [ 2 ] == '3' ) )
		return 0;
	int flags = buffer [ 5 ];
	int unsynchronisation = flags & 0x80,
		extendedHeader = flags & 0x40,
		experimentalIndicator = flags & 0x20,
		footerPresent = flags & 0x10;

	uint8_t * id3TagSize = buffer + 6;

	return ( ( id3TagSize [ 0 ] & 0x7f ) << 21 ) + ( ( id3TagSize [ 1 ] & 0x7f ) << 14 )
		+ ( ( id3TagSize [ 2 ] & 0x7f ) << 7 ) + ( id3TagSize [ 3 ] & 0x7f )
		+ 10 + ( footerPresent ? 10 : 0 );
}

// Get ID3 v1.x Tag Length
inline int get_id3v1_length ( uint8_t * buffer, int maxSize )
{
	if ( !( buffer [ 0 ] == 'T' && buffer [ 1 ] == 'A' && buffer [ 2 ] == 'G' ) )
		return 0;
	return 128;
}

// Get Chunk Size from MP3 Header
inline int get_mp3_header_chunk_size ( uint32_t headerValue, int * channels, int * samplerate )
{
	int bitrateTable [ 4 ] [ 4 ] [ 16 ] = {
		{																							// kMEPGVERSION_2_5
			{ 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0 },			//<< kMPEGLAYER_RESERVED
			{ 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1 },			//<< kMPEGLAYER_III
			{ 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1 },			//<< kMPEGLAYER_II
			{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, -1 },			//<< kMPEGLAYER_I
		},
		{																							// kMPEGVERSION_RESERVED
			{ 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0 },			//<< kMPEGLAYER_RESERVED
			{ 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0 },			//<< kMPEGLAYER_III
			{ 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0 },			//<< kMPEGLAYER_II
			{ 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0 },			//<< kMPEGLAYER_I
		},
		{																							// kMPEGVERSION_2
			{ 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0 },			//<< kMPEGLAYER_RESERVED
			{ 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1 },			//<< kMPEGLAYER_III
			{ 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1 },			//<< kMPEGLAYER_II
			{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, -1 },			//<< kMPEGLAYER_I
		},
		{																							// kMPEGVERSION_1
			{ 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0 },			//<< kMPEGLAYER_RESERVED
			{ 0, 32, 40, 45,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, -1 },			//<< kMPEGLAYER_III
			{ 0, 32, 45, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, -1 },			//<< kMPEGLAYER_II
			{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1 },			//<< kMPEGLAYER_I
		}
	};

	int frequencyTable [ 4 ] [ 4 ] = {
		{ 11025, 12000,  8000, 0 },																	// kMEPGVERSION_2_5
	{ 0,     0,         0, 0 },																	// kMPEGVERSION_RESERVED
	{ 22050, 24000, 16000, 0 },																	// kMEPGVERSION_2
	{ 44100, 48000, 32000, 0 }																	// kMEPGVERSION_1
	};

	enum : uint8_t {
		kMEPGVERSION_2_5 = 0x0,
		kMPEGVERSION_RESERVED = 0x1,
		kMPEGVERSION_2 = 0x2,
		kMPEGVERSION_1 = 0x3,
	};

	enum : uint8_t {
		kMPEGLAYER_RESERVED = 0x0,
		kMPEGLAYER_III = 0x1,
		kMPEGLAYER_II = 0x2,
		kMPEGLAYER_I = 0x3,
	};

	enum : uint8_t {
		kCHANNELS_STEREO = 0x0,
		kCHANNELS_JOINT_STEREO = 0x1,
		kCHANNELS_DUAL_CHANNEL = 0x2,
		kCHANNELS_SINGLE_CHANNEL = 0x3,
	};

	union MP3FrameHeader
	{
#pragma pack ( push, 1 )
		struct
		{
			struct {
				uint8_t emphasis : 2;
				bool original : 1;
				bool copyright : 1;
				uint8_t mode : 2;
				uint8_t channels : 2;
			};
			struct {
				bool priv : 1;
				bool padding : 1;
				uint8_t frequency : 2;
				uint8_t bitrate : 4;
			};
			struct {
				bool errorProtection : 1;
				uint8_t layer : 2;
				uint8_t version : 2;
				uint8_t syncWord2 : 3;
			};
			uint8_t syncWord1;
		} header;
#pragma pack ( pop )
		int32_t value;
	} header;
	header.value = _byteswap_ulong ( headerValue );

	if ( header.header.syncWord1 != 0xff || header.header.syncWord2 != 0x7 )
		return 0;
	if ( header.header.version == kMPEGVERSION_RESERVED )
		return 0;
	if ( header.header.layer == kMPEGLAYER_RESERVED )
		return 0;

	int bitrate = bitrateTable [ header.header.version ] [ header.header.layer ] [ header.header.bitrate ] * 1000;
	int frequency = frequencyTable [ header.header.version ] [ header.header.frequency ];

	if ( channels != nullptr )
		*channels = header.header.channels != kCHANNELS_SINGLE_CHANNEL ? 2 : 1;
	if ( samplerate != nullptr )
		*samplerate = frequency;

	return 144 * bitrate / frequency + header.header.padding;
}

struct POSITIONSIZE { int64_t position, size; };

#include <vector>
// Cache MP3 Chunk positions
inline void cache_chunk_position ( AESTREAM * stream, std::vector<POSITIONSIZE> & cache, int * channels, int * samplerate )
{
	uint8_t readBuffer [ 12 ];
	int64_t readed;

	int64_t length = stream->length ( stream->object );
	if ( length == -1 )
		return;

	do
	{
		int64_t position = stream->tell ( stream->object );

		if ( position >= length )
			break;

		readed = stream->read ( stream->object, readBuffer, 12 );
		if ( readed == 0 )
			return;

		int64_t size = get_mp3_header_chunk_size ( *( ( int32_t* ) readBuffer ), channels, samplerate );
		if ( size != 0 )
		{
			POSITIONSIZE posize = { position, size };
			cache.push_back ( posize );
			stream->seek ( stream->object, size - 12, AESO_CURRENT );
		}
		else
		{
			size = get_id3v2_length ( readBuffer, 12 );
			if ( size != 0 )
			{
				stream->seek ( stream->object, size - 12, AESO_CURRENT );
			}
			else
			{
				size = get_id3v2_length ( readBuffer, 12 );
				if ( size != 0 )
					stream->seek ( stream->object, size - 12, AESO_CURRENT );
				else
					break;
			}
		}
	} while ( true );
}

#endif