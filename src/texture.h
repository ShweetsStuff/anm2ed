#pragma once

#include "log.h"

#define TEXTURE_CHANNELS 4
#define TEXTURE_INIT_INFO "Initialized texture from file: {}"
#define TEXTURE_INIT_ERROR "Failed to initialize texture from file: {}"
#define TEXTURE_SAVE_INFO "Saved texture to: {}"
#define TEXTURE_SAVE_ERROR "Failed to save texture to: {}"

struct Texture {
  GLuint id = GL_ID_NONE;
  ivec2 size{};
  bool isInvalid = true;

  auto operator<=>(const Texture&) const = default;
};

bool texture_from_gl_write(Texture* self, const std::string& path);
bool texture_from_path_init(Texture* self, const std::string& path);
bool texture_from_rgba_init(Texture* self, ivec2 size, const uint8_t* data);
bool texture_from_rgba_write(const std::string& path, const uint8_t* data, ivec2 size);
bool texture_pixel_set(Texture* self, ivec2 position, vec4 color);
void texture_free(Texture* self);
bool texture_from_memory_init(Texture* self, ivec2 size, const uint8_t* data, size_t length);
std::vector<uint8_t> texture_download(const Texture* self);