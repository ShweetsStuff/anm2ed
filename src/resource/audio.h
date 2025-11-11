#pragma once

#include <SDL3_mixer/SDL_mixer.h>

namespace anm2ed::resource
{
  class Audio
  {
    MIX_Audio* internal{nullptr};
    MIX_Mixer* mixer_get();
    void unload();

  public:
    Audio(const char*);
    ~Audio();
    Audio() = default;
    Audio(Audio&&) noexcept;
    Audio& operator=(Audio&&) noexcept;
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool is_valid();
    void play(MIX_Mixer* = nullptr);
  };
}
