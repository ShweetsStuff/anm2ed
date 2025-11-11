#include "audio.h"

#include <SDL3/SDL_properties.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#ifdef _MSC_VER
  #ifndef __attribute__
    #define __attribute__(x)
  #endif
  #ifndef restrict
    #define restrict __restrict
  #endif
#endif

#include <xm.h>

namespace anm2ed::resource
{
  namespace
  {
    constexpr int XM_SAMPLE_RATE = 44100;
    constexpr uint16_t XM_CHUNK_FRAMES = 1024;
    constexpr int XM_MAX_SECONDS = 600;
    constexpr int XM_CHANNELS = 2;
  }

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
    if (!data || size == 0) return;

    auto is_chunk_silent = [](const float* samples, size_t count)
    {
      constexpr float epsilon = 1e-6f;
      for (size_t i = 0; i < count; ++i)
        if (std::fabs(samples[i]) > epsilon) return false;
      return true;
    };

    auto prescanStorage = std::make_unique<unsigned char[]>(XM_PRESCAN_DATA_SIZE);
    auto prescan = (xm_prescan_data_t*)prescanStorage.get();
    if (!xm_prescan_module((const char*)data, (uint32_t)size, prescan)) return;

    auto contextSize = xm_size_for_context(prescan);
    auto pool = std::make_unique<char[]>(contextSize);
    auto context = xm_create_context(pool.get(), prescan, (const char*)data, (uint32_t)size);
    if (!context) return;

    xm_set_sample_rate(context, (uint16_t)XM_SAMPLE_RATE);
    xm_set_max_loop_count(context, 1);

    auto pcm = std::vector<float>{};
    pcm.reserve(XM_CHUNK_FRAMES * XM_CHANNELS * 8);

    auto framesGenerated = (size_t)0;
    const auto maxFrames = (size_t)XM_SAMPLE_RATE * XM_MAX_SECONDS;
    auto heardAudio = false;

    while (framesGenerated < maxFrames)
    {
      auto framesThisPass = (uint16_t)std::min<size_t>(XM_CHUNK_FRAMES, maxFrames - framesGenerated);
      auto offset = pcm.size();
      pcm.resize(offset + framesThisPass * XM_CHANNELS);
      auto* chunkStart = pcm.data() + offset;
      xm_generate_samples(context, chunkStart, framesThisPass);
      framesGenerated += framesThisPass;

      auto chunkSamples = (size_t)framesThisPass * XM_CHANNELS;
      auto chunkSilent = is_chunk_silent(chunkStart, chunkSamples);
      if (!chunkSilent)
      {
        heardAudio = true;
      }
      else if (heardAudio)
      {
        pcm.resize(offset);
        break;
      }

      if (xm_get_loop_count(context) > 0) break;
    }

    if (pcm.empty()) return;

    auto spec = SDL_AudioSpec{};
    spec.freq = XM_SAMPLE_RATE;
    spec.format = SDL_AUDIO_F32;
    spec.channels = XM_CHANNELS;

    internal = MIX_LoadRawAudio(nullptr, pcm.data(), pcm.size() * sizeof(float), &spec);
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
