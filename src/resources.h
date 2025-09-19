#pragma once

#include "RESOURCE.h"
#include "shader.h"
#include "texture.h"

#define RESOURCES_TEXTURES_FREE_INFO "Freed texture resources"

struct Resources {
  GLuint shaders[SHADER_COUNT];
  Texture atlas;
};

void resources_init(Resources* self);
void resources_free(Resources* self);
