#include "resources.h"
#include "RESOURCE.h"

void resources_init(Resources* self) {
  texture_from_memory_init(&self->atlas, TEXTURE_ATLAS_SIZE, TEXTURE_ATLAS, TEXTURE_ATLAS_LENGTH);

  for (int i = 0; i < SHADER_COUNT; i++)
    shader_init(&self->shaders[i], SHADER_DATA[i].vertex, SHADER_DATA[i].fragment);
}

void resources_free(Resources* self) {
  for (auto& shader : self->shaders)
    shader_free(&shader);

  texture_free(&self->atlas);
}