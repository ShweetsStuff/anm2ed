#include "audio.h"

#include <utility>

namespace anm2ed::resource
{
  MIX_Mixer* Audio::mixer_get()
  {
    static MIX_Mixer* mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    return mixer;
  }

  Audio::Audio(const char* path)
  {
    if (path && *path) internal = MIX_LoadAudio(mixer_get(), path, true);
  }

  void Audio::unload()
  {
    if (!internal) return;
    MIX_DestroyAudio(internal);
    internal = nullptr;
  }

  void Audio::play()
  {
    MIX_PlayAudio(mixer_get(), internal);
  }

  Audio::Audio(Audio&& other) noexcept
  {
    internal = std::exchange(other.internal, nullptr);
  }

  Audio& Audio::operator=(Audio&& other) noexcept
  {
    if (this != &other)
    {
      unload();
      internal = std::exchange(other.internal, nullptr);
    }
    return *this;
  }

  Audio::~Audio()
  {
    unload();
  }
}
