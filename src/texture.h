#pragma once

#include "log.h"

#define TEXTURE_CHANNELS 4
#define TEXTURE_INIT_INFO "Initialized texture from file: {}"
#define TEXTURE_INIT_ERROR "Failed to initialize texture from file: {}"
#define TEXTURE_SAVE_INFO "Saved texture to: {}"
#define TEXTURE_SAVE_ERROR "Failed to save texture to: {}"

struct Texture
{
    GLuint id = GL_ID_NONE;
    ivec2 size{};
    bool isInvalid = true;
};

bool texture_from_encoded_data_init(Texture* self, ivec2 size, const u8* data, u32 length);
bool texture_from_gl_write(Texture* self, const std::string& path);
bool texture_from_path_init(Texture* self, const std::string& path);
bool texture_from_rgba_init(Texture* self, ivec2 size, const u8* data);
bool texture_from_rgba_write(const std::string& path, const u8* data, ivec2 size);
bool texture_pixel_set(Texture* self, ivec2 position, vec4 color);
void texture_free(Texture* self);
std::vector<u8> texture_download(const Texture* self);