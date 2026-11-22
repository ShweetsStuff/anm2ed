#include "playback.h"

#include <cmath>

#include "vec.h"

void Playback::toggle()
{
    if (isFinished)
        time = 0.0f;
    isFinished = false;
    isPlaying = !isPlaying;
}

void Playback::clamp(int length)
{
    time = vec_clamp(time, 0.0f, (float)length - 1.0f);
}

// Plays the frames [start, end). A playhead outside them starts at start.
void Playback::tick(int fps, int start, int end, PlaybackLoop loop, float deltaSeconds)
{
    if (loop == PLAYBACK_LOOP)
        isFinished = false;
    if (isFinished || !isPlaying || fps <= 0 || end <= start)
        return;
    if (deltaSeconds <= 0.0f)
        return;

    if (!std::isfinite(time) || time < (float)start || time >= (float)end)
        time = (float)start;
    time += deltaSeconds * (float)fps;

    if (loop == PLAYBACK_LOOP)
    {
        time = (float)start + std::fmod(time - (float)start, (float)(end - start));
        return;
    }

    if (time >= (float)end)
    {
        time = (float)end - 1.0f;
        isPlaying = false;
        isFinished = true;
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
