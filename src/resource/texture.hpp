#pragma once

#include <glad/glad.h>

#include "image.hpp"

// GPU copies of Images, uploaded on first use and evicted when a frame passes without them being drawn.
namespace anm2ed::resource::texture
{
  GLuint id_get(const Image&);
  void garbage_collect();
  Image svg_load(const char*, size_t, glm::ivec2);
}
