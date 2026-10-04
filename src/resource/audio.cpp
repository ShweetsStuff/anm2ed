#include "audio.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <unordered_map>

namespace anm2ed::resource::audio
{
  struct Entry
  {
    MIX_Audio* audio{};
    MIX_Track* track{};
    std::atomic<float> squares[2]{};
    std::atomic<int> sampleCount{};
  };

  // Sums the squares of the left/right samples the track plays, for its levels.
  void SDLCALL levels_update(void* userdata, MIX_Track*, const SDL_AudioSpec* spec, float* pcm, int samples)
  {
    auto entry = (Entry*)userdata;
    auto channels = std::max(spec->channels, 1);
    float squares[2]{};
    for (int i = 0; i < samples; ++i)
      squares[std::min(i % channels, 1)] += pcm[i] * pcm[i];
    if (channels == 1) squares[1] = squares[0];
    for (int channel = 0; channel < 2; ++channel)
      entry->squares[channel] += squares[channel];
    entry->sampleCount += samples / channels;
  }

  std::unordered_map<std::uint64_t, Entry> entries{};

  MIX_Mixer* mixer_get()
  {
    static auto mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    return mixer;
  }

  Entry* entry_get(const AudioData& data)
  {
    if (!data.is_valid()) return nullptr;
    auto [it, isNew] = entries.try_emplace(data.uid);
    if (isNew)
      if (auto io = SDL_IOFromConstMem(data.bytes.data(), data.bytes.size()))
        it->second.audio = MIX_LoadAudio_IO(mixer_get(), io, true, true);
    return it->second.audio ? &it->second : nullptr;
  }

  // The playing track, when it belongs to mixer (or any mixer, when none is given).
  MIX_Track* track_get(const AudioData& data, MIX_Mixer* mixer)
  {
    auto it = entries.find(data.uid);
    if (it == entries.end() || !it->second.track) return nullptr;
    return !mixer || MIX_GetTrackMixer(it->second.track) == mixer ? it->second.track : nullptr;
  }

  void play(const AudioData& data, bool isLoop, MIX_Mixer* mixer)
  {
    auto entry = entry_get(data);
    auto targetMixer = mixer ? mixer : mixer_get();
    if (!entry || !targetMixer) return;

    if (entry->track && MIX_GetTrackMixer(entry->track) != targetMixer) track_detach(data);
    if (!entry->track) entry->track = MIX_CreateTrack(targetMixer);
    if (!entry->track) return;
    MIX_SetTrackCookedCallback(entry->track, levels_update, entry);

    MIX_SetTrackAudio(entry->track, entry->audio);

    SDL_PropertiesID options = isLoop ? SDL_CreateProperties() : 0;
    if (options) SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    MIX_PlayTrack(entry->track, options);
    if (options) SDL_DestroyProperties(options);
  }

  void stop(const AudioData& data, MIX_Mixer* mixer)
  {
    if (auto track = track_get(data, mixer)) MIX_StopTrack(track, 0);
  }

  void track_detach(const AudioData& data, MIX_Mixer* mixer)
  {
    if (!track_get(data, mixer)) return;
    auto& track = entries[data.uid].track;
    MIX_DestroyTrack(track);
    track = nullptr;
  }

  bool is_playing(const AudioData& data)
  {
    auto track = track_get(data, nullptr);
    return track && MIX_TrackPlaying(track);
  }

  // The left/right loudness (RMS, 0-1) of what the track played since the last call.
  glm::vec2 levels_get(const AudioData& data)
  {
    auto it = entries.find(data.uid);
    if (it == entries.end()) return {};
    auto& entry = it->second;
    auto count = (float)std::max(entry.sampleCount.exchange(0), 1);
    return {std::sqrt(entry.squares[0].exchange(0.0f) / count), std::sqrt(entry.squares[1].exchange(0.0f) / count)};
  }
}
