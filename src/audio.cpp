#include "audio.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <memory>
#include <unordered_map>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

constexpr SDL_AudioSpec AUDIO_SPEC = {SDL_AUDIO_F32, AUDIO_CHANNELS, AUDIO_FREQUENCY};
constexpr int FRAME_SIZE = AUDIO_CHANNELS * sizeof(float);

// One play of a sound. The audio thread feeds its stream from `position`, wrapping around when it loops.
struct AudioVoice
{
    struct AudioSound* sound = nullptr;
    SDL_AudioStream* stream = nullptr;
    std::atomic<std::size_t> position = 0;
    std::atomic<bool> isLoop = false;
};

// A decoded sound, its voices (one per overlapping play, reused once finished) and the loudness they played.
struct AudioSound
{
    std::vector<float> samples;
    std::vector<std::unique_ptr<AudioVoice>> voices;
    std::atomic<float> squares[AUDIO_CHANNELS] = {};
    std::atomic<int> frameCount = 0;
};

static std::unordered_map<std::uint64_t, AudioSound> sounds;

// Converts audio in `spec` to AUDIO_SPEC.
static std::vector<float> samples_convert(const SDL_AudioSpec& spec, const void* data, int size)
{
    Uint8* converted = nullptr;
    int convertedSize = 0;
    if (!SDL_ConvertAudioSamples(&spec, (const Uint8*)data, size, &AUDIO_SPEC, &converted, &convertedSize))
        return {};
    std::vector<float> samples(convertedSize / sizeof(float));
    std::memcpy(samples.data(), converted, samples.size() * sizeof(float));
    SDL_free(converted);
    return samples;
}

static std::vector<float> samples_decode(const AudioData& data)
{
    if (data.bytes.size() >= 4 && std::memcmp(data.bytes.data(), "OggS", 4) == 0)
    {
        int channels = 0;
        int frequency = 0;
        short* decoded = nullptr;
        auto frames =
            stb_vorbis_decode_memory(data.bytes.data(), (int)data.bytes.size(), &channels, &frequency, &decoded);
        if (frames <= 0)
            return {};
        auto samples =
            samples_convert({SDL_AUDIO_S16, channels, frequency}, decoded, frames * channels * sizeof(short));
        free(decoded);
        return samples;
    }

    SDL_AudioSpec spec = {};
    Uint8* decoded = nullptr;
    Uint32 size = 0;
    if (!SDL_LoadWAV_IO(SDL_IOFromConstMem(data.bytes.data(), data.bytes.size()), true, &spec, &decoded, &size))
        return {};
    auto samples = samples_convert(spec, decoded, (int)size);
    SDL_free(decoded);
    return samples;
}

static AudioSound* sound_get(const AudioData& data)
{
    if (!data.is_valid())
        return nullptr;
    auto [it, isNew] = sounds.try_emplace(data.uid);
    if (isNew)
        it->second.samples = samples_decode(data);
    return it->second.samples.empty() ? nullptr : &it->second;
}

// Opened on the first play. A failed open is tried again on the next, for a device that shows up later.
static SDL_AudioDeviceID device_get()
{
    static SDL_AudioDeviceID device = 0;
    if (!device && SDL_InitSubSystem(SDL_INIT_AUDIO))
        device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &AUDIO_SPEC);
    return device;
}

// Runs on the audio thread: gives the stream what it asks for, and adds it to the sound's loudness.
static void SDLCALL voice_feed(void* userdata, SDL_AudioStream* stream, int amount, int)
{
    auto voice = (AudioVoice*)userdata;
    auto& samples = voice->sound->samples;
    auto frames = (std::size_t)amount / FRAME_SIZE;
    auto position = voice->position.load();
    float squares[AUDIO_CHANNELS] = {};
    std::size_t fed = 0;
    while (fed < frames)
    {
        if (position * AUDIO_CHANNELS >= samples.size())
        {
            if (!voice->isLoop)
                break;
            position = 0;
        }
        auto count = std::min(frames - fed, samples.size() / AUDIO_CHANNELS - position);
        auto start = samples.data() + position * AUDIO_CHANNELS;
        for (std::size_t i = 0; i < count * AUDIO_CHANNELS; ++i)
            squares[i % AUDIO_CHANNELS] += start[i] * start[i];
        SDL_PutAudioStreamData(stream, start, (int)(count * FRAME_SIZE));
        position += count;
        fed += count;
    }
    voice->position = position;
    for (int channel = 0; channel < AUDIO_CHANNELS; ++channel)
        voice->sound->squares[channel] += squares[channel];
    voice->sound->frameCount += (int)fed;
}

static bool is_voice_playing(AudioVoice& voice)
{
    return voice.isLoop || voice.position * AUDIO_CHANNELS < voice.sound->samples.size() ||
           SDL_GetAudioStreamQueued(voice.stream) > 0;
}

// A sound can play over itself: while a voice is still playing it, another voice takes the new play.
void audio_play(const AudioData& data, AudioPlayMode mode)
{
    auto sound = sound_get(data);
    if (!sound || !device_get())
        return;

    auto free = std::ranges::find_if(sound->voices, [](auto& voice) { return !is_voice_playing(*voice); });
    AudioVoice* voice = nullptr;
    if (free != sound->voices.end())
        voice = free->get();
    else
    {
        auto stream = SDL_CreateAudioStream(&AUDIO_SPEC, &AUDIO_SPEC);
        if (!stream)
            return;
        voice = sound->voices.emplace_back(std::make_unique<AudioVoice>()).get();
        voice->sound = sound;
        voice->stream = stream;
        SDL_SetAudioStreamGetCallback(voice->stream, voice_feed, voice);
        SDL_BindAudioStream(device_get(), voice->stream);
    }

    SDL_LockAudioStream(voice->stream);
    SDL_ClearAudioStream(voice->stream);
    voice->position = 0;
    voice->isLoop = mode == AUDIO_LOOP;
    SDL_UnlockAudioStream(voice->stream);
}

void audio_stop(const AudioData& data)
{
    auto it = sounds.find(data.uid);
    if (it == sounds.end())
        return;
    for (auto& voice : it->second.voices)
    {
        SDL_LockAudioStream(voice->stream);
        voice->isLoop = false;
        voice->position = it->second.samples.size() / AUDIO_CHANNELS;
        SDL_ClearAudioStream(voice->stream);
        SDL_UnlockAudioStream(voice->stream);
    }
}

bool is_audio_playing(const AudioData& data)
{
    auto it = sounds.find(data.uid);
    return it != sounds.end() &&
           std::ranges::any_of(it->second.voices, [](auto& voice) { return is_voice_playing(*voice); });
}

// The left/right loudness (RMS, 0-1) of what the sound played since the last call.
Vec2 audio_levels_get(const AudioData& data)
{
    auto it = sounds.find(data.uid);
    if (it == sounds.end())
        return {};
    auto& sound = it->second;
    auto count = (float)std::max(sound.frameCount.exchange(0), 1);
    return {std::sqrt(sound.squares[0].exchange(0.0f) / count), std::sqrt(sound.squares[1].exchange(0.0f) / count)};
}

std::vector<float> audio_mix(const std::vector<std::vector<const AudioData*>>& frameSounds, int fps)
{
    std::vector<float> mix;
    if (fps <= 0)
        return mix;
    auto framesPerStep = (double)AUDIO_FREQUENCY / fps;
    for (std::size_t step = 0; step < frameSounds.size(); ++step)
    {
        auto start = (std::size_t)std::floor(step * framesPerStep) * AUDIO_CHANNELS;
        mix.resize(std::max(mix.size(), (std::size_t)std::floor((step + 1) * framesPerStep) * AUDIO_CHANNELS));
        for (auto data : frameSounds[step])
        {
            auto sound = sound_get(*data);
            if (!sound)
                continue;
            mix.resize(std::max(mix.size(), start + sound->samples.size()));
            for (std::size_t i = 0; i < sound->samples.size(); ++i)
                mix[start + i] += sound->samples[i];
        }
    }
    for (auto& sample : mix)
        sample = std::clamp(sample, -1.0f, 1.0f);
    return mix;
}
