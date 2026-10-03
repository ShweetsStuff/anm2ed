#include "texture.hpp"

#include <lunasvg.h>
#include <memory>
#include <unordered_map>
#include <utility>

namespace anm2ed::resource::texture
{
  struct Entry
  {
    GLuint id{};
    bool isUsed{};
  };

  std::unordered_map<std::uint64_t, Entry> entries{};

  GLuint id_get(const Image& image)
  {
    if (!image.is_valid()) return 0;

    auto [it, isNew] = entries.try_emplace(image.uid);
    auto& entry = it->second;
    entry.isUsed = true;
    if (!isNew) return entry.id;

    auto filter = image.isLinear ? GL_LINEAR : GL_NEAREST;
    glGenTextures(1, &entry.id);
    glBindTexture(GL_TEXTURE_2D, entry.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.size.x, image.size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image.pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return entry.id;
  }

  void garbage_collect()
  {
    std::erase_if(entries,
                  [](auto& pair)
                  {
                    auto& entry = pair.second;
                    if (!entry.isUsed) glDeleteTextures(1, &entry.id);
                    return !std::exchange(entry.isUsed, false);
                  });
  }

  Image svg_load(const char* data, size_t length, glm::ivec2 size)
  {
    auto document = data ? lunasvg::Document::loadFromData(data, length) : nullptr;
    if (!document) return {};
    auto bitmap = document->renderToBitmap(size.x, size.y, 0);
    if (bitmap.width() == 0 || bitmap.height() == 0) return {};
    return Image(bitmap.data(), size, true);
  }
}
