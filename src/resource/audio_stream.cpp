#include "audio_stream.h"

#if defined(__clang__) || defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

namespace anm2ed
{
  void AudioStream::callback(void* userData, MIX_Mixer* mixer, const SDL_AudioSpec* spec, float* pcm, int samples)
  {
    auto self = (AudioStream*)userData;
    self->stream.insert(self->stream.end(), pcm, pcm + samples);
  }

  AudioStream::AudioStream(MIX_Mixer* mixer) { MIX_GetMixerFormat(mixer, &spec); }

  void AudioStream::capture_begin(MIX_Mixer* mixer) { MIX_SetPostMixCallback(mixer, callback, this); }

  void AudioStream::capture_end(MIX_Mixer* mixer)
  {
    MIX_SetPostMixCallback(mixer, nullptr, this);
    stream.clear();
  }
}