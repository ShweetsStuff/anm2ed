#pragma once

#include "PACKED.h"
#include "texture.h"
#include "shader.h"

#define RESOURCES_TEXTURES_FREE_INFO "Freed texture resources"

struct Resources
{
    GLuint shaders[SHADER_COUNT];
    Texture atlas;
};

void resources_init(Resources* self);
void resources_free(Resources* self);
