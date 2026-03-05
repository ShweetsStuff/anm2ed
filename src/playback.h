#pragma once

namespace anm2ed
{
  class Playback
  {
  public:
    float time{};
    bool isPlaying{};
    bool isFinished{};

    void toggle();
    void timing_reset();
    void clamp(int);
    void tick(int, int, bool, float);
    void decrement(int);
    void increment(int);

  private:
    float tickAccumulator{};
  };
}
