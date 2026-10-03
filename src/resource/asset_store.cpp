#include "asset_store.hpp"

#include <span>

namespace anm2ed::resource
{
  constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ull;
  constexpr std::uint64_t FNV_PRIME = 1099511628211ull;

  std::uint64_t bytes_hash(std::span<const std::uint8_t> bytes, std::uint64_t hash = FNV_OFFSET)
  {
    for (auto byte : bytes)
      hash = (hash ^ byte) * FNV_PRIME;
    return hash;
  }

  template <class Value> std::span<const std::uint8_t> value_bytes_get(const Value& value)
  {
    return {reinterpret_cast<const std::uint8_t*>(&value), sizeof(value)};
  }

  // Content hash; 0 stays free to mean "no asset".
  std::uint64_t hash_finish(std::uint64_t hash) { return hash ? hash : 1; }

  std::uint64_t AssetStore::image_add(Image image)
  {
    auto hash = bytes_hash(value_bytes_get(image.size));
    hash = bytes_hash(value_bytes_get(image.isLinear), hash);
    hash = hash_finish(bytes_hash(image.pixels, hash));
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
}
