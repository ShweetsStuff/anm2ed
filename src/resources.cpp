#include "resources.h"

// Loads a texture, given a path and an id for it to be assigned to
void
resources_texture_init(Resources* resources, const std::string& path, s32 id)
{
	Texture texture;

	if (resources->textures.find(id) != resources->textures.end() && resources->textures[id].id != resources->textures[TEXTURE_ERROR].id)
		texture_free(&resources->textures[id]);

	if (texture_from_path_init(&texture, path))
		resources->textures[id] = texture;
	else
		texture.isInvalid = true;

	resources->textures[id] = texture;
}

// Loads in resources
void
resources_init(Resources* self)
{
    texture_from_data_init(&self->atlas, (u8*)TEXTURE_ATLAS, TEXTURE_ATLAS_LENGTH);

    for (s32 i = 0; i < SHADER_COUNT; i++)
        shader_init(&self->shaders[i], SHADER_DATA[i].vertex, SHADER_DATA[i].fragment);
}

// Frees resources
void
resources_free(Resources* self)
{
    resources_textures_free(self);

    for (s32 i = 0; i < SHADER_COUNT; i++)
        shader_free(&self->shaders[i]);
        
    texture_free(&self->atlas);
}

// Frees loaded textures
void
resources_textures_free(Resources* self)
{
    for (auto & [id, texture] : self->textures)
        texture_free(&self->textures[id]);
}