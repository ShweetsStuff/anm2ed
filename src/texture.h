#pragma once

#include "COMMON.h"

struct Texture
{
    GLuint id = 0;
    ivec2 size = {0, 0};
    s32 channels = -1;
    bool isInvalid = false;
};

void texture_gl_set(Texture* self, void* data);
bool texture_from_path_init(Texture* self, const std::string& path);
bool texture_from_data_init(Texture* self, const u8* data, u32 length);
void texture_free(Texture* self);
bool texture_from_data_write(const std::string& path, const u8* data, s32 width, s32 height);