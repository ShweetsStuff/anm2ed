#pragma once

#include "vec.h"
#include <vector>

#include "audio_data.h"

constexpr int AUDIO_CHANNELS = 2;
constexpr int AUDIO_FREQUENCY = 48000;

enum AudioPlayMode
{
    AUDIO_ONCE,
    AUDIO_LOOP
};

// Sounds (WAV or Ogg Vorbis) are decoded on their first play to AUDIO_FREQUENCY stereo floats, cached by uid.
// Each play is a stream on one shared device, opened on the first play, which mixes them.
void audio_play(const AudioData&, AudioPlayMode = AUDIO_ONCE);
void audio_stop(const AudioData&);
bool is_audio_playing(const AudioData&);
Vec2 audio_levels_get(const AudioData&);

// Each frame's sounds started on that frame and mixed at `fps`, as interleaved stereo floats.
std::vector<float> audio_mix(const std::vector<std::vector<const AudioData*>>&, int fps);
