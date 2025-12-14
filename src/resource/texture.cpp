#include "texture.h"

#include <filesystem>
#include <fstream>
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

#include "math_.h"

using namespace anm2ed::resource::texture;
using namespace anm2ed::util::math;
using namespace glm;

namespace
{
  std::vector<uint8_t> file_read_all(const std::filesystem::path& path)
  {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    if (size <= 0) return {};

    std::vector<uint8_t> buffer(static_cast<std::size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size))) return {};

    return buffer;
  }
}

namespace anm2ed::resource
{
  bool Texture::is_valid() const { return id != 0; }

  size_t Texture::pixel_size_get() const { return size.x * size.y * CHANNELS; }

  void Texture::upload(const uint8_t* data)
  {
    if (!data || size.x <= 0 || size.y <= 0) return;

    if (!is_valid()) glGenTextures(1, &id);

    pixels.assign(data, data + pixel_size_get());

    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    glBindTexture(GL_TEXTURE_2D, 0);
  }

  Texture::Texture() = default;

  Texture::~Texture()
  {
    if (is_valid()) glDeleteTextures(1, &id);
  }

  Texture::Texture(const Texture& other) { *this = other; }

  Texture::Texture(Texture&& other) { *this = std::move(other); }

  Texture& Texture::operator=(const Texture& other)
  {
    if (this != &other)
    {
      if (is_valid()) glDeleteTextures(1, &id);
      size = other.size;
      filter = other.filter;
      channels = other.channels;
      pixels = other.pixels;
      upload(pixels.data());
    }
    return *this;
  }

  Texture& Texture::operator=(Texture&& other)
  {
    if (this != &other)
    {
      if (is_valid()) glDeleteTextures(1, &id);
      id = other.id;
      size = other.size;
      filter = other.filter;
      channels = other.channels;
      pixels = std::move(other.pixels);
      other.id = 0;
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

  Texture::Texture(const std::filesystem::path& pngPath)
  {
    auto fileData = file_read_all(pngPath);
    if (fileData.empty()) return;

    if (const uint8_t* data =
            stbi_load_from_memory(fileData.data(), static_cast<int>(fileData.size()), &size.x, &size.y, nullptr,
                                  CHANNELS);
        data)
    {
      upload(data);
      stbi_image_free((void*)data);
    }
  }

  bool Texture::write_png(const std::filesystem::path& path)
  {
    if (pixels.empty()) return false;

    std::ofstream file(path, std::ios::binary);
    if (!file) return false;

    auto write_func = [](void* context, void* data, int size) {
      auto* stream = static_cast<std::ofstream*>(context);
      stream->write(static_cast<char*>(data), size);
    };

    auto result = stbi_write_png_to_func(write_func, &file, size.x, size.y, CHANNELS, this->pixels.data(),
                                         size.x * CHANNELS);
    file.flush();
    return result != 0;
  }

  vec4 Texture::pixel_read(vec2 position) const
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

  void Texture::pixel_set(ivec2 position, vec4 color)
  {
    if (position.x < 0 || position.y < 0 || position.x >= size.x || position.y >= size.y) return;

    uint8 rgba8[4] = {(uint8)float_to_uint8(color.r), (uint8)float_to_uint8(color.g), (uint8)float_to_uint8(color.b),
                      (uint8)float_to_uint8(color.a)};

    if (is_valid())
    {
      glBindTexture(GL_TEXTURE_2D, id);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, position.x, position.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba8);
      glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (pixels.size() == pixel_size_get())
    {
      size_t idx = (position.y * size.x + position.x) * CHANNELS;
      memcpy(&pixels[idx], rgba8, 4);
    }
  }

  void Texture::pixel_line(ivec2 start, ivec2 end, vec4 color)
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
