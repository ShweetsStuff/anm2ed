#pragma once

#include "COMMON.h"

struct Texture
{
    GLuint handle = 0;
    ivec2 size = {0, 0};
    s32 channels = -1;
};

bool texture_init(Texture* self, const char* path);
void texture_free(Texture* self);
