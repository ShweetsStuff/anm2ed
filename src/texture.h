#pragma once

#include "COMMON.h"

#define TEXTURE_CHANNELS 4
#define TEXTURE_INIT_INFO "Initialized texture from file: {}"
#define TEXTURE_INIT_ERROR "Failed to initialize texture from file: {}"
#define TEXTURE_SAVE_INFO "Saved texture to: {}"

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
std::vector<u8> texture_download(Texture* self);
bool texture_from_data_write(const std::string& path, const u8* data, ivec2 size);
bool texture_pixel_set(Texture* self, ivec2 position, vec4 color);
bool texture_from_gl_write(Texture* self, const std::string& path);