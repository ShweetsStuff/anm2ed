#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

namespace anm2ed::resource::image
{
  constexpr auto CHANNELS = 4;
}

namespace anm2ed::resource
{
  // CPU-side RGBA pixels. uid changes whenever the pixels do, so GPU copies can be cached by it.
  class Image
  {
  public:
    glm::ivec2 size{};
    std::vector<uint8_t> pixels{};
    bool isLinear{};
    std::uint64_t uid{};

    Image() = default;
    Image(const uint8_t*, glm::ivec2, bool = false);
    Image(const unsigned char*, size_t);
    Image(const std::filesystem::path&);

    bool is_valid() const;
    size_t pixel_size_get() const;
    bool write_png(const std::filesystem::path&) const;
    static bool write_pixels_png(const std::filesystem::path&, glm::ivec2, const uint8_t*);
    static Image merge_append(const Image&, const Image&, bool);
    glm::vec4 pixel_read(glm::vec2) const;
    void pixel_set(glm::ivec2, glm::vec4);
    void pixel_line(glm::ivec2, glm::ivec2, glm::vec4);
  };
}
