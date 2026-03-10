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

  void AudioStream::capture_begin(MIX_Mixer* mixer)
  {
    stream.clear();
    callbackSamples = 0;
    captureStartCounter = SDL_GetPerformanceCounter();
    firstCallbackCounter = 0;
    isFirstCallbackCaptured = false;
    MIX_SetPostMixCallback(mixer, callback, this);
  }

  void AudioStream::capture_end(MIX_Mixer* mixer)
  {
    MIX_SetPostMixCallback(mixer, nullptr, this);
    stream.clear();
  }

  double AudioStream::callback_latency_seconds_get() const
  {
    auto freq = std::max(spec.freq, 1);
    auto channels = std::max(spec.channels, 1);
    auto framesPerCallback = (double)callbackSamples / (double)channels;
    return framesPerCallback / (double)freq;
  }

  double AudioStream::capture_start_delay_seconds_get() const
  {
    if (!isFirstCallbackCaptured || captureStartCounter == 0 || firstCallbackCounter < captureStartCounter) return 0.0;
    auto frequency = SDL_GetPerformanceFrequency();
    if (frequency == 0) return 0.0;
    return (double)(firstCallbackCounter - captureStartCounter) / (double)frequency;
  }
}
