#pragma once

#include <SDL3_mixer/SDL_mixer.h>
#include <glm/vec2.hpp>

#include "audio_data.h"

// Playback of AudioData. Decoded audio and tracks are cached per AudioData uid.
void audio_play(const AudioData&, bool = false, MIX_Mixer* = nullptr);
void audio_stop(const AudioData&, MIX_Mixer* = nullptr);
void audio_track_detach(const AudioData&, MIX_Mixer* = nullptr);
bool is_audio_playing(const AudioData&);
glm::vec2 audio_levels_get(const AudioData&);
