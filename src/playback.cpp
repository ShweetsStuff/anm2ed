#include "playback.h"

#include <algorithm>
#include <cmath>

#include <glm/common.hpp>

namespace anm2ed
{
  void Playback::toggle()
  {
    if (isFinished) time = 0.0f;
    isFinished = false;
    isPlaying = !isPlaying;
    timing_reset();
  }

  void Playback::timing_reset() { tickAccumulator = 0.0f; }

  void Playback::clamp(int length) { time = glm::clamp(time, 0.0f, (float)length - 1.0f); }

  void Playback::tick(int fps, int length, bool isLoop, float deltaSeconds)
  {
    if (isFinished || !isPlaying || fps <= 0 || length <= 0) return;
    if (deltaSeconds <= 0.0f) return;

    auto frameDuration = 1.0f / (float)fps;
    tickAccumulator += deltaSeconds;
    auto steps = (int)std::floor(tickAccumulator / frameDuration);
    if (steps <= 0) return;
    tickAccumulator -= frameDuration * (float)steps;

    time += (float)steps;

    if (time > (float)length - 1.0f)
    {
      if (isLoop)
      {
        time = std::fmod(time, (float)length);
      }
      else
      {
        time = (float)length - 1.0f;
        isPlaying = false;
        isFinished = true;
        timing_reset();
      }
    }
  }

  void Playback::decrement(int length)
  {
    --time;
    clamp(length);
    timing_reset();
  }

  void Playback::increment(int length)
  {
    ++time;
    clamp(length);
    timing_reset();
  }
}
