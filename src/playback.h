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
    void clamp(int);
    void tick(int, int, bool);
    void decrement(int);
    void increment(int);
  };
}
