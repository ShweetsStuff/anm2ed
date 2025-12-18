#include "audio.h"

#include <cstdio>
#include <utility>

#include "file_.h"

using namespace anm2ed::util;

namespace anm2ed::resource
{
  MIX_Mixer* Audio::mixer_get()
  {
    static auto mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    return mixer;
  }

  Audio::Audio(const std::filesystem::path& path)
  {
    if (path.empty()) return;

    File file(path, "rb");
    if (!file) return;

    if (std::fseek(file.get(), 0, SEEK_END) != 0) return;
    auto size = std::ftell(file.get());
    if (size <= 0)
    {
      return;
    }
    std::rewind(file.get());

    data.resize(static_cast<std::size_t>(size));
    auto read = std::fread(data.data(), 1, data.size(), file.get());
    if (read == 0)
    {
      data.clear();
      return;
    }
    data.resize(read);

    SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());
    if (!io)
      data.clear();
    else
    {
      internal = MIX_LoadAudio_IO(mixer_get(), io, true, true);
      if (!internal) data.clear();
    }
  }

  Audio::Audio(const unsigned char* memory, size_t size)
  {
    if (!memory || size == 0) return;
    data.assign(memory, memory + size);

    SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());
    if (!io)
      data.clear();
    else
    {
      internal = MIX_LoadAudio_IO(mixer_get(), io, true, true);
      if (!internal) data.clear();
    }
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

  Audio::Audio(const Audio& other)
  {
    if (other.data.empty()) return;

    data = other.data;
    SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());
    if (!io)
      data.clear();
    else
    {
      internal = MIX_LoadAudio_IO(mixer_get(), io, true, true);
      if (!internal) data.clear();
    }
  }

  Audio::Audio(Audio&& other) noexcept
  {
    internal = std::exchange(other.internal, nullptr);
    track = std::exchange(other.track, nullptr);
    data = std::move(other.data);
  }

  Audio& Audio::operator=(Audio&& other) noexcept
  {
    if (this != &other)
    {
      unload();
      internal = std::exchange(other.internal, nullptr);
      track = std::exchange(other.track, nullptr);
      data = std::move(other.data);
    }
    return *this;
  }

  Audio& Audio::operator=(const Audio& other)
  {
    if (this != &other)
    {
      unload();
      data.clear();
      if (!other.data.empty())
      {
        data = other.data;
        SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());
        if (!io)
          data.clear();
        else
        {
          internal = MIX_LoadAudio_IO(mixer_get(), io, true, true);
          if (!internal) data.clear();
        }
      }
    }
    return *this;
  }

  Audio::~Audio() { unload(); }
  bool Audio::is_valid() { return internal; }
}
