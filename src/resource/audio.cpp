#include "audio.h"

#include <SDL3/SDL_properties.h>
#include <utility>

namespace anm2ed::resource
{
  MIX_Mixer* Audio::mixer_get()
  {
    static auto mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    return mixer;
  }

  Audio::Audio(const char* path)
  {
    if (path && *path) internal = MIX_LoadAudio(mixer_get(), path, true);
  }

  Audio::Audio(const unsigned char* data, size_t size)
  {
    SDL_IOStream* io = SDL_IOFromConstMem(data, size);
    if (!io) return;
    internal = MIX_LoadAudio_IO(mixer_get(), io, true, true);
  }

  void Audio::unload()
  {
    if (track)
    {
      MIX_DestroyTrack(track);
      track = nullptr;
    }
    if (internal)
    {
      MIX_DestroyAudio(internal);
      internal = nullptr;
    }
  }

  void Audio::play(bool loop, MIX_Mixer* mixer)
  {
    if (!internal) return;
    auto targetMixer = mixer ? mixer : mixer_get();
    if (!targetMixer) return;

    if (track && MIX_GetTrackMixer(track) != targetMixer)
    {
      MIX_DestroyTrack(track);
      track = nullptr;
    }

    if (!track)
    {
      track = MIX_CreateTrack(targetMixer);
      if (!track) return;
    }

    MIX_SetTrackAudio(track, internal);

    SDL_PropertiesID options = 0;
    if (loop)
    {
      options = SDL_CreateProperties();
      if (options) SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    }

    MIX_PlayTrack(track, options);

    if (options) SDL_DestroyProperties(options);
  }

  void Audio::stop(MIX_Mixer* mixer)
  {
    if (!track) return;
    if (mixer && MIX_GetTrackMixer(track) != mixer) return;
    MIX_StopTrack(track, 0);
  }

  bool Audio::is_playing() const { return track && MIX_TrackPlaying(track); }

  Audio::Audio(Audio&& other) noexcept
  {
    internal = std::exchange(other.internal, nullptr);
    track = std::exchange(other.track, nullptr);
  }

  Audio& Audio::operator=(Audio&& other) noexcept
  {
    if (this != &other)
    {
      unload();
      internal = std::exchange(other.internal, nullptr);
      track = std::exchange(other.track, nullptr);
    }
    return *this;
  }

  Audio::~Audio() { unload(); }
  bool Audio::is_valid() { return internal; }
}
