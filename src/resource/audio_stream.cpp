#include "audio_stream.hpp"

#include <algorithm>

#if defined(__clang__) || defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

namespace anm2ed
{
  void AudioStream::callback(void* userData, MIX_Mixer* mixer, const SDL_AudioSpec* spec, float* pcm, int samples)
  {
    auto self = (AudioStream*)userData;
    if (!self->isFirstCallbackCaptured)
    {
      self->firstCallbackCounter = SDL_GetPerformanceCounter();
      self->isFirstCallbackCaptured = true;
    }
    self->callbackSamples = samples;
    self->stream.insert(self->stream.end(), pcm, pcm + samples);
  }

  AudioStream::AudioStream(MIX_Mixer* mixer) { MIX_GetMixerFormat(mixer, &spec); }

}
