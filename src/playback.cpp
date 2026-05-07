#include "playback.hpp"

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
    if (isLoop) isFinished = false;
    if (isFinished || !isPlaying || fps <= 0 || length <= 0) return;
    if (deltaSeconds <= 0.0f) return;

    time += deltaSeconds * (float)fps;

    if (!std::isfinite(time)) time = 0.0f;

    if (isLoop)
    {
      time = std::fmod(time, (float)length);
      if (time < 0.0f) time += (float)length;
      return;
    }

    if (time >= (float)length)
    {
      time = (float)length - 1.0f;
      isPlaying = false;
      isFinished = true;
      timing_reset();
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
