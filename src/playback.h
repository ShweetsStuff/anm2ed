#pragma once

class Playback
{
public:
    float time = 0;
    bool isPlaying = false;
    bool isFinished = false;

    void toggle();
    void timing_reset();
    void clamp(int);
    void tick(int, int, int, bool, float);
    void decrement(int);
    void increment(int);

private:
    float tickAccumulator = 0;
};
