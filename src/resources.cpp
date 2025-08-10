#include "resources.h"

void resources_texture_init(Resources* self, const std::string& path, s32 id)
{
	Texture texture;

    if (map_find(self->textures, id))
		texture_free(&self->textures[id]);

	texture_from_path_init(&texture, path);

    self->textures[id] = texture;
}

void resources_init(Resources* self)
{
    texture_from_encoded_data_init(&self->atlas, TEXTURE_ATLAS_SIZE, TEXTURE_CHANNELS, (u8*)TEXTURE_ATLAS, TEXTURE_ATLAS_LENGTH);
    
    for (s32 i = 0; i < SHADER_COUNT; i++) 
        shader_init(&self->shaders[i], SHADER_DATA[i].vertex, SHADER_DATA[i].fragment);
}

void resources_free(Resources* self)
{
    resources_textures_free(self);
    
    for (auto& shader : self->shaders)
        shader_free(&shader);

    texture_free(&self->atlas);
}

void resources_textures_free(Resources* self)
{
    for (auto& [id, texture] : self->textures) 
        texture_free(&self->textures[id]);
    
    log_info(RESOURCES_TEXTURES_FREE_INFO);
}