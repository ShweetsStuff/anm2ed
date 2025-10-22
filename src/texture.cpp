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

using namespace glm;

namespace anm2ed::texture
{
  bool Texture::is_valid()
  {
    return id != 0;
  }

  void Texture::download(std::vector<uint8_t>& pixels)
  {
    pixels.resize(size.x * size.y * CHANNELS);
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void Texture::init(const uint8_t* data, bool isDownload)
  {
    glGenTextures(1, &id);

    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    if (isDownload) download(pixels);
  }

  Texture::Texture() = default;

  Texture::~Texture()
  {
    if (is_valid()) glDeleteTextures(1, &id);
  }

  Texture::Texture(Texture&& other)
  {
    *this = std::move(other);
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
    init(bitmap.data());
  }

  Texture::Texture(const std::string& pngPath, bool isDownload)
  {
    if (const uint8* data = stbi_load(pngPath.c_str(), &size.x, &size.y, nullptr, CHANNELS); data)
    {
      init(data, isDownload);
      stbi_image_free((void*)data);
    }
  }

  bool Texture::write_png(const std::string& path)
  {
    std::vector<uint8_t> pixels;
    download(pixels);
    const bool isSuccess = stbi_write_png(path.c_str(), size.x, size.y, CHANNELS, pixels.data(), size.x * CHANNELS);
    return isSuccess;
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