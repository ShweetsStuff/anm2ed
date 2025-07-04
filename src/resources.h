#pragma once

#include "PACKED.h"
#include "texture.h"
#include "shader.h"

struct Resources
{
    GLuint shaders[SHADER_COUNT];
    Texture atlas;
    std::map<s32, Texture> textures;
};

void resources_init(Resources* self);
void resources_texture_init(Resources* resources, const std::string& path, s32 id);
void resources_free(Resources* self);
void resources_textures_free(Resources* self);
