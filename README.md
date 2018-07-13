# AudEar
Audio Decoding and Playing Library for C++.

## libAudEar
libAudEar is Audio Decoding and Playing Interface Library.

### Support Decoders
* Media Foundation Audio Decoder

### Support Players
* XAudio 2 Audio Player
* Windows Audio Session API Audio Player
  * Shared Mode Player
  * Exclusive Mode Player

### Plans
* CoreAudio Audio Decoder
* DirectSound 8 Audio Player
* WaveOut Audio Player
* ALSA Audio Player
* OpenAL Audio Player
* SIMD support Converter Streams for faster
* Multi-channels audio to Mono audio Converter Stream
* Resampler Audio Stream for Crossplatform
* 3D Effect Audio Player
  * DirectSound 8
  * XAudio 2
  * OpenAL

## libAudEar.OSSCodec
libAudEar.OSSCodec is Open Source Software Decoder Library. This library to redistribution under LGPL.

### Support Decoders
* OggVorbis Audio Decoder
* OggOpus Audio Decoder
* OggFLAC and FLAC Audio Decoder
  * OggFLAC Audio Decoder is not support Seeking and getting Duration.
* MP3 Audio Decoder

### Plans
* WavPack Audio Decoder
* M4A and MKA contains AAC Audio Decoder
* ALAC Audio Decoder
* ASF and WMA Audio Decoder

## libAudEar.Filter
libAudEar.Filter is Audio Filtering Library.

### Support Filters
* BiQuad Filtering Audio Stream
  * High Pass Filter
  * Low Pass Filter
  * High Shelf Filter
  * Low Shelf Filter
  * All Pass Filter
  * Notch Filter
  * Peak Eq Filter
  * Band Pass Filter(Constant Skirt Gain)
  * Band Pass Filter(Constant Peak Gain)
* Equalizer Initializer
  * Equalizer Presets

### Plans
* Frequency Spectrum Analyzer