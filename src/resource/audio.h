#pragma once

#include <SDL3_mixer/SDL_mixer.h>
#include <cstddef>
#include <filesystem>
#include <vector>

namespace anm2ed::resource
{
  class Audio
  {
    MIX_Audio* internal{nullptr};
    MIX_Track* track{nullptr};
    std::vector<unsigned char> data{};
    MIX_Mixer* mixer_get();
    void unload();

  public:
    Audio(const unsigned char*, size_t);
    Audio(const std::filesystem::path&);
    ~Audio();
    Audio() = default;
    Audio(const Audio&);
    Audio(Audio&&) noexcept;
    Audio& operator=(const Audio&);
    Audio& operator=(Audio&&) noexcept;

    bool is_valid();
    void play(bool loop = false, MIX_Mixer* = nullptr);
    void stop(MIX_Mixer* = nullptr);
    bool is_playing() const;
  };
}
