#pragma once

#include <SDL3_mixer/SDL_mixer.h>
#include <filesystem>
#include <string>
#include <cstddef>

namespace anm2ed::resource
{
  class Audio
  {
    MIX_Audio* internal{nullptr};
    MIX_Track* track{nullptr};
    MIX_Mixer* mixer_get();
    void unload();

  public:
    Audio(const char*);
    Audio(const unsigned char*, size_t);
    Audio(const std::string&);
    Audio(const std::filesystem::path&);
    ~Audio();
    Audio() = default;
    Audio(Audio&&) noexcept;
    Audio& operator=(Audio&&) noexcept;
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool is_valid();
    void play(bool loop = false, MIX_Mixer* = nullptr);
    void stop(MIX_Mixer* = nullptr);
    bool is_playing() const;
  };
}
