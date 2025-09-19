#include "resources.h"

void resources_init(Resources* self) {
  texture_from_path_init(&self->atlas, ATLAS_PATH);

  for (int i = 0; i < SHADER_COUNT; i++)
    shader_init(&self->shaders[i], SHADER_DATA[i].vertex, SHADER_DATA[i].fragment);
}

void resources_free(Resources* self) {
  for (auto& shader : self->shaders)
    shader_free(&shader);

  texture_free(&self->atlas);
}