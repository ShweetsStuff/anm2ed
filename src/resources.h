#pragma once

#include "RESOURCES.h"
#include "texture.h"
#include "shader.h"

struct Resources
{
    Texture textures[TEXTURE_COUNT];
    GLuint shaders[SHADER_COUNT];
    std::map<s32, Texture> loadedTextures;
};

void resources_init(Resources* self);
void resources_free(Resources* self);
void resources_loaded_textures_free(Resources* self);
