#include "asset_store.h"

#include <bit>
#include <cstring>
#include <span>

constexpr std::uint64_t HASH_SEED = 14695981039346656037ull;
constexpr std::uint64_t HASH_SPREAD = 0x9E3779B97F4A7C15ull;
constexpr std::uint64_t HASH_MIX = 0x94D049BB133111EBull;
constexpr int HASH_ROTATION = 27;

// Hashes eight bytes a step (the tail one at a time), each mixed by a multiply and a rotation. Hashes stay within a
// session, so they only have to tell contents apart.
static std::uint64_t bytes_hash(std::span<const std::uint8_t> bytes, std::uint64_t hash = HASH_SEED)
{
    auto step = [&](std::uint64_t word) {
        hash = std::rotl(hash ^ (word * HASH_SPREAD), HASH_ROTATION) * HASH_MIX;
    };
    std::size_t i = 0;
    for (; i + sizeof(std::uint64_t) <= bytes.size(); i += sizeof(std::uint64_t))
    {
        std::uint64_t word = 0;
        std::memcpy(&word, bytes.data() + i, sizeof(word));
        step(word);
    }
    for (; i < bytes.size(); ++i)
        step(bytes[i]);
    step(bytes.size());
    return hash;
}

template <class Value> static std::span<const std::uint8_t> value_bytes_get(const Value& value)
{
    return {reinterpret_cast<const std::uint8_t*>(&value), sizeof(value)};
}

// Content hash. 0 stays free to mean "no asset".
static std::uint64_t hash_finish(std::uint64_t hash)
{
    return hash ? hash : 1;
}

std::uint64_t AssetStore::image_add(Image image)
{
    auto hash = bytes_hash(value_bytes_get(image.size));
    hash = bytes_hash(value_bytes_get(image.isLinear), hash);
    hash = bytes_hash(image.pixels, hash);
    if (image.svg)
        hash = bytes_hash({(const std::uint8_t*)image.svg->data(), image.svg->size()}, hash);
    hash = hash_finish(hash);
    images.try_emplace(hash, std::move(image));
    return hash;
}

std::uint64_t AssetStore::audio_add(AudioData data)
{
    auto hash = hash_finish(bytes_hash(data.bytes));
    audio.try_emplace(hash, std::move(data));
    return hash;
}

const Image* AssetStore::image_get(std::uint64_t hash) const
{
    auto it = images.find(hash);
    return it == images.end() ? nullptr : &it->second;
}

const AudioData* AssetStore::audio_get(std::uint64_t hash) const
{
    auto it = audio.find(hash);
    return it == audio.end() ? nullptr : &it->second;
}
