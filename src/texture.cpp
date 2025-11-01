#include "texture.h"

#include <lunasvg.h>
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

#include "math.h"

using namespace anm2ed::math;
using namespace glm;

namespace anm2ed::texture
{
  bool Texture::is_valid()
  {
    return id != 0;
  }

  std::vector<uint8_t> Texture::pixels_get()
  {
    ensure_pixels();
    return pixels;
  }

  void Texture::download()
  {
    if (size.x <= 0 || size.y <= 0 || !is_valid()) return;
    pixels.resize(static_cast<size_t>(size.x) * static_cast<size_t>(size.y) * CHANNELS);
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    isPixelsDirty = false;
  }

  void Texture::upload(const uint8_t* data)
  {
    if (!data || size.x <= 0 || size.y <= 0) return;

    const size_t pixelCount = static_cast<size_t>(size.x) * static_cast<size_t>(size.y) * CHANNELS;
    pixels.assign(data, data + pixelCount);
    upload();
  }

  void Texture::upload()
  {
    if (pixels.empty() || size.x <= 0 || size.y <= 0) return;

    if (!is_valid()) glGenTextures(1, &id);

    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    isPixelsDirty = false;
  }

  Texture::Texture() = default;

  Texture::~Texture()
  {
    if (is_valid()) glDeleteTextures(1, &id);
  }

  Texture::Texture(const Texture& other)
  {
    *this = other;
  }

  Texture::Texture(Texture&& other)
  {
    *this = std::move(other);
  }

  Texture& Texture::operator=(const Texture& other)
  {
    if (this != &other)
    {
      if (is_valid()) glDeleteTextures(1, &id);
      id = 0;
      other.ensure_pixels();
      size = other.size;
      filter = other.filter;
      channels = other.channels;
      pixels = other.pixels;
      if (!pixels.empty()) upload();
    }
    return *this;
  }

  Texture& Texture::operator=(Texture&& other)
  {
    if (this != &other)
    {
      if (is_valid()) glDeleteTextures(1, &id);
      id = std::exchange(other.id, 0);
      size = std::exchange(other.size, {});
      filter = other.filter;
      channels = other.channels;
      pixels = std::move(other.pixels);
      isPixelsDirty = other.isPixelsDirty;
      other.isPixelsDirty = true;
      if (!pixels.empty()) upload();
    }
    return *this;
  }

  Texture::Texture(const char* svgData, size_t svgDataLength, ivec2 svgSize)
  {
    if (!svgData) return;

    const std::unique_ptr<lunasvg::Document> document = lunasvg::Document::loadFromData(svgData, svgDataLength);
    if (!document) return;

    const lunasvg::Bitmap bitmap = document->renderToBitmap(svgSize.x, svgSize.y, 0);
    if (bitmap.width() == 0 || bitmap.height() == 0) return;

    size = svgSize;
    filter = GL_LINEAR;
    upload(bitmap.data());
  }

  Texture::Texture(const uint8_t* data, ivec2 size)
  {
    this->size = size;
    upload(data);
  }

  Texture::Texture(const std::string& pngPath)
  {
    if (const uint8_t* data = stbi_load(pngPath.c_str(), &size.x, &size.y, nullptr, CHANNELS); data)
    {
      upload(data);
      stbi_image_free((void*)data);
    }
  }

  bool Texture::write_png(const std::string& path)
  {
    ensure_pixels();
    return stbi_write_png(path.c_str(), size.x, size.y, CHANNELS, this->pixels.data(), size.x * CHANNELS);
  }

  void Texture::pixel_set(ivec2 position, vec4 color)
  {
    if (position.x < 0 || position.y < 0 || position.x >= size.x || position.y >= size.y) return;

    ensure_pixels();
    uint8 rgba8[4] = {(uint8)float_to_uint8(color.r), (uint8)float_to_uint8(color.g), (uint8)float_to_uint8(color.b),
                      (uint8)float_to_uint8(color.a)};

    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, position.x, position.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba8);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (!pixels.empty())
    {
      auto index = (position.y * size.x + position.x) * CHANNELS;
      pixels[index + 0] = rgba8[0];
      pixels[index + 1] = rgba8[1];
      pixels[index + 2] = rgba8[2];
      pixels[index + 3] = rgba8[3];
      isPixelsDirty = false;
    }
    else
      isPixelsDirty = true;
  }

  void Texture::pixel_line(ivec2 start, ivec2 end, vec4 color)
  {
    ensure_pixels();
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

  void Texture::ensure_pixels() const
  {
    if (size.x <= 0 || size.y <= 0) return;
    if (!pixels.empty() && !isPixelsDirty) return;
    const_cast<Texture*>(this)->download();
  }

  void Texture::bind(GLuint unit)
  {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
  }

  void Texture::unbind(GLuint unit)
  {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
}
