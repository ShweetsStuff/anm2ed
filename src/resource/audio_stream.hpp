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

    AudioStream(MIX_Mixer*);
    void capture_begin(MIX_Mixer*);
    void capture_end(MIX_Mixer*);
  };
}