#include "playback.hpp"

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

  // Plays the frames [start, end); a playhead outside them starts at start.
  void Playback::tick(int fps, int start, int end, bool isLoop, float deltaSeconds)
  {
    if (isLoop) isFinished = false;
    if (isFinished || !isPlaying || fps <= 0 || end <= start) return;
    if (deltaSeconds <= 0.0f) return;

    if (!std::isfinite(time) || time < (float)start || time >= (float)end) time = (float)start;
    time += deltaSeconds * (float)fps;

    if (isLoop)
    {
      time = (float)start + std::fmod(time - (float)start, (float)(end - start));
      return;
    }

    if (time >= (float)end)
    {
      time = (float)end - 1.0f;
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
