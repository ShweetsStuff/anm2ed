#pragma once

#include <SDL3_mixer/SDL_mixer.h>
#include <vector>

namespace anm2ed
{
  class AudioStream
  {
    static void callback(void*, MIX_Mixer*, const SDL_AudioSpec*, float*, int);

  public:
    std::vector<float> stream{};
    SDL_AudioSpec spec{};
    int callbackSamples{};
    Uint64 captureStartCounter{};
    Uint64 firstCallbackCounter{};
    bool isFirstCallbackCaptured{};

    AudioStream(MIX_Mixer*);
    void capture_begin(MIX_Mixer*);
    void capture_end(MIX_Mixer*);
    double callback_latency_seconds_get() const;
    double capture_start_delay_seconds_get() const;
  };
}
