#pragma once

#include <SDL3_mixer/SDL_mixer.h>
#include <vector>

  class AudioStream
  {
    static void callback(void*, MIX_Mixer*, const SDL_AudioSpec*, float*, int);

  public:
    std::vector<float> stream;
    SDL_AudioSpec spec = {};
    int callbackSamples = 0;
    Uint64 captureStartCounter = 0;
    Uint64 firstCallbackCounter = 0;
    bool isFirstCallbackCaptured = false;

    AudioStream(MIX_Mixer*);
  };
