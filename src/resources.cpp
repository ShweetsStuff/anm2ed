#include "resources.h"

/* Loads in resources */
void
resources_init(Resources* self)
{
    texture_from_data_init(&self->atlas, (u8*)TEXTURE_ATLAS, TEXTURE_ATLAS_LENGTH);

    for (s32 i = 0; i < SHADER_COUNT; i++)
        shader_init(&self->shaders[i], SHADER_DATA[i].vertex, SHADER_DATA[i].fragment);
}

/* Frees resources*/
void
resources_free(Resources* self)
{
    resources_textures_free(self);

    for (s32 i = 0; i < SHADER_COUNT; i++)
        shader_free(&self->shaders[i]);
        
    texture_free(&self->atlas);
}

/* Frees loaded textures */
void
resources_textures_free(Resources* self)
{
    for (auto & [id, texture] : self->textures)
        texture_free(&self->textures[id]);
}