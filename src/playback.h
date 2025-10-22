#pragma once

namespace anm2ed::playback
{
  class Playback
  {
  public:
    float time{};
    bool isPlaying{};
    bool isFinished{};

    void toggle();
    void clamp(int length);
    void tick(int fps, int length, bool isLoop);
    void decrement(int length);
    void increment(int length);
  };
}