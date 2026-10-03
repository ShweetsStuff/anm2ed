#include "image.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <utility>

#if defined(__clang__) || defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  #pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#if defined(__clang__) || defined(__GNUC__)
  #pragma GCC diagnostic pop
#endif

#include "file.hpp"
#include "math.hpp"
#include "uid.hpp"

using namespace anm2ed::resource::image;
using namespace anm2ed::util::math;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::resource
{
  bool Image::is_valid() const { return !pixels.empty(); }

  size_t Image::pixel_size_get() const { return size.x * size.y * CHANNELS; }

  Image::Image(const uint8_t* data, ivec2 size, bool isLinear) : isLinear(isLinear)
  {
    if (!data || size.x <= 0 || size.y <= 0) return;
    this->size = size;
    pixels.assign(data, data + pixel_size_get());
    uid = uid_next();
  }

  Image::Image(const unsigned char* pngData, size_t pngSize)
  {
    if (!pngData || pngSize == 0) return;

    auto sizeInt = static_cast<int>(pngSize);
    if (sizeInt <= 0) return;

    if (auto data = stbi_load_from_memory(pngData, sizeInt, &size.x, &size.y, nullptr, CHANNELS); data)
    {
      *this = Image(data, size);
      stbi_image_free((void*)data);
    }
  }

  Image::Image(const std::filesystem::path& pngPath)
  {
    File file(pngPath, "rb");
    if (auto handle = file.get())
    {
      if (auto data = stbi_load_from_file(handle, &size.x, &size.y, nullptr, CHANNELS); data)
      {
        *this = Image(data, size);
        stbi_image_free((void*)data);
      }
    }
  }

  bool Image::write_png(const std::filesystem::path& path) const
  {
    return write_pixels_png(path, size, pixels.empty() ? nullptr : pixels.data());
  }

  bool Image::write_pixels_png(const std::filesystem::path& path, ivec2 size, const uint8_t* data)
  {
    if (!data || size.x <= 0 || size.y <= 0) return false;

    File file(path, "wb");
    if (auto handle = file.get())
    {
      auto write_func = [](void* context, void* bytes, int count)
      { fwrite(bytes, 1, count, static_cast<FILE*>(context)); };
      return stbi_write_png_to_func(write_func, handle, size.x, size.y, CHANNELS, data, size.x * CHANNELS) != 0;
    }
    return false;
  }

  // Copies a size-sized block between images; pixels outside either image are skipped.
  void pixels_copy(const Image& from, glm::ivec2 fromMin, Image& to, glm::ivec2 toMin, glm::ivec2 size)
  {
    for (int y = 0; y < size.y; ++y)
      for (int x = 0; x < size.x; ++x)
      {
        auto source = fromMin + glm::ivec2(x, y);
        auto target = toMin + glm::ivec2(x, y);
        if (glm::any(glm::lessThan(source, glm::ivec2(0))) || glm::any(glm::greaterThanEqual(source, from.size)) ||
            glm::any(glm::lessThan(target, glm::ivec2(0))) || glm::any(glm::greaterThanEqual(target, to.size)))
          continue;
        std::copy_n(from.pixels.data() + ((std::size_t)source.y * from.size.x + source.x) * image::CHANNELS,
                    image::CHANNELS,
                    to.pixels.data() + ((std::size_t)target.y * to.size.x + target.x) * image::CHANNELS);
      }
  }

  void Image::paste(const Image& source, glm::ivec2 position) { pixels_copy(source, {}, *this, position, source.size); }

  // A copy of a block of the image (transparent where it falls outside).
  Image Image::region_get(glm::ivec2 min, glm::ivec2 regionSize) const
  {
    Image region{};
    region.size = regionSize;
    region.pixels.assign((std::size_t)regionSize.x * regionSize.y * image::CHANNELS, 0);
    pixels_copy(*this, min, region, {}, regionSize);
    return region;
  }

  Image Image::merge_append(const Image& base, const Image& append, bool isAppendRight)
  {
    if (base.size.x <= 0 || base.size.y <= 0) return append;
    if (append.size.x <= 0 || append.size.y <= 0) return base;
    if (base.pixels.empty()) return append;
    if (append.pixels.empty()) return base;

    auto resultSize = isAppendRight ? ivec2(base.size.x + append.size.x, std::max(base.size.y, append.size.y))
                                    : ivec2(std::max(base.size.x, append.size.x), base.size.y + append.size.y);
    auto resultPixelsSize = (size_t)resultSize.x * (size_t)resultSize.y * CHANNELS;
    std::vector<uint8_t> resultPixels(resultPixelsSize, 0);

    auto blit = [&](const Image& texture, ivec2 offset)
    {
      for (int y = 0; y < texture.size.y; y++)
      {
        auto src = (size_t)y * (size_t)texture.size.x * CHANNELS;
        auto dst = ((size_t)(offset.y + y) * (size_t)resultSize.x + (size_t)offset.x) * CHANNELS;
        std::memcpy(resultPixels.data() + dst, texture.pixels.data() + src, (size_t)texture.size.x * CHANNELS);
      }
    };

    blit(base, {0, 0});
    blit(append, isAppendRight ? ivec2(base.size.x, 0) : ivec2(0, base.size.y));

    return Image(resultPixels.data(), resultSize);
  }

  vec4 Image::pixel_read(vec2 position) const
  {
    if (pixels.size() < CHANNELS || size.x <= 0 || size.y <= 0) return vec4(0.0f);

    int x = glm::clamp((int)(position.x), 0, size.x - 1);
    int y = glm::clamp((int)(position.y), 0, size.y - 1);

    auto index = ((size_t)(y) * (size_t)(size.x) + (size_t)(x)) * CHANNELS;
    if (index + CHANNELS > pixels.size()) return vec4(0.0f);

    vec4 color{uint8_to_float(pixels[index + 0]), uint8_to_float(pixels[index + 1]), uint8_to_float(pixels[index + 2]),
               uint8_to_float(pixels[index + 3])};

    if (color.a <= 0.0f) color = vec4(0.0f);
    return color;
  }

  void Image::pixel_set(ivec2 position, vec4 color)
  {
    if (position.x < 0 || position.y < 0 || position.x >= size.x || position.y >= size.y) return;

    uint8 rgba8[4] = {(uint8)float_to_uint8(color.r), (uint8)float_to_uint8(color.g), (uint8)float_to_uint8(color.b),
                      (uint8)float_to_uint8(color.a)};

    if (pixels.size() != pixel_size_get()) return;
    size_t idx = (position.y * size.x + position.x) * CHANNELS;
    memcpy(&pixels[idx], rgba8, 4);
    uid = uid_next();
  }

  void Image::pixel_line(ivec2 start, ivec2 end, vec4 color)
  {
    auto plot = [&](ivec2 pos) { pixel_set(pos, color); };

    int x0 = start.x;
    int y0 = start.y;
    int x1 = end.x;
    int y1 = end.y;

    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
      plot({x0, y0});
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy)
      {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx)
      {
        err += dx;
        y0 += sy;
      }
    }
  }
}
