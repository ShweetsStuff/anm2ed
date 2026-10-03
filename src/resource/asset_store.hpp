#pragma once

#include <cstdint>
#include <unordered_map>

#include "audio_data.hpp"
#include "image.hpp"

namespace anm2ed::resource
{
  // Immutable images and sounds keyed by content hash. Undo history records keys, never pixels or bytes, and
  // entries live until the document closes so any step can be restored.
  class AssetStore
  {
    std::unordered_map<std::uint64_t, Image> images{};
    std::unordered_map<std::uint64_t, AudioData> audio{};

  public:
    std::uint64_t image_add(Image);
    std::uint64_t audio_add(AudioData);
    const Image* image_get(std::uint64_t) const;
    const AudioData* audio_get(std::uint64_t) const;
  };
}
