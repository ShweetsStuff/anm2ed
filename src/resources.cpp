#include "resources.h"

/* Loads in resources */
void
resources_init(Resources* self)
{
    /* Textures */
    for (s32 i = 0; i < TEXTURE_COUNT; i++)
        texture_from_data_init(&self->textures[i], (u8*)TEXTURE_DATA[i].data, TEXTURE_DATA[i].length);

    for (s32 i = 0; i < SHADER_COUNT; i++)
        shader_init(&self->shaders[i], SHADER_DATA[i].vertex, SHADER_DATA[i].fragment);
}

/* Frees resources*/
void
resources_free(Resources* self)
{
    for (s32 i = 0; i < TEXTURE_COUNT; i++)
        texture_free(&self->textures[i]);

    for (s32 i = 0; i < SHADER_COUNT; i++)
        shader_free(&self->shaders[i]);

    resources_loaded_textures_free(self);
}

void
resources_loaded_textures_free(Resources* self)
{
    for (auto & [id, texture] : self->loadedTextures)
        texture_free(&self->textures[id]);
}