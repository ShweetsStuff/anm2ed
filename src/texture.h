#pragma once

#include <glad/glad.h>

#include "image.h"

// GPU copies of Images, uploaded on first use and evicted when a frame passes without them being drawn.
GLuint texture_id_get(const Image&);
void texture_garbage_collect();
Image texture_svg_load(const char*, size_t, glm::ivec2);
