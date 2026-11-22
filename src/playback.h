#pragma once

enum PlaybackLoop
{
    PLAYBACK_ONCE,
    PLAYBACK_LOOP
};

class Playback
{
public:
    float time = 0;
    bool isPlaying = false;
    bool isFinished = false;

    void toggle();
    void clamp(int);
    void tick(int, int, int, PlaybackLoop, float);
    void decrement(int);
    void increment(int);
};
