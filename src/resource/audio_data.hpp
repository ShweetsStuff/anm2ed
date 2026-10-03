#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace anm2ed::resource
{
  // Encoded audio file bytes. uid identifies the bytes so playback state can be cached by it.
  struct AudioData
  {
    std::vector<unsigned char> bytes{};
    std::uint64_t uid{};

    AudioData() = default;
    AudioData(const unsigned char*, size_t);
    AudioData(const std::filesystem::path&);

    bool is_valid() const;
  };
}
