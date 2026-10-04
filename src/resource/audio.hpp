#pragma once

#include <SDL3_mixer/SDL_mixer.h>
#include <glm/vec2.hpp>

#include "audio_data.hpp"

// Playback of AudioData; decoded audio and tracks are cached per AudioData uid.
namespace anm2ed::resource::audio
{
  void play(const AudioData&, bool = false, MIX_Mixer* = nullptr);
  void stop(const AudioData&, MIX_Mixer* = nullptr);
  void track_detach(const AudioData&, MIX_Mixer* = nullptr);
  bool is_playing(const AudioData&);
  glm::vec2 levels_get(const AudioData&);
}
