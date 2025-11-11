#include "playback.h"

#include <glm/common.hpp>

namespace anm2ed
{
  void Playback::toggle()
  {
    if (isFinished) time = 0.0f;
    isFinished = false;
    isPlaying = !isPlaying;
  }

  void Playback::clamp(int length)
  {
    time = glm::clamp(time, 0.0f, (float)length - 1.0f);
  }

  void Playback::tick(int fps, int length, bool isLoop)
  {
    if (isFinished) return;

    time += (float)fps / 30.0f;

    if (time >= (float)length)
    {
      if (isLoop)
        time = 0.0f;
      else
      {
        isPlaying = false;
        isFinished = true;
      }
    }
  }

  void Playback::decrement(int length)
  {
    --time;
    clamp(length);
  }

  void Playback::increment(int length)
  {
    ++time;
    clamp(length);
  }
}