#include "texture.h"

#define STBI_ONLY_PNG  
#define STBI_NO_FAILURE_STRINGS   
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

void
texture_gl_set(Texture* self, void* data)
{
	glGenTextures(1, &self->id);
	
	glBindTexture(GL_TEXTURE_2D, self->id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->size.x, self->size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0); 
}

bool
texture_from_path_init(Texture* self, const char* path)
{
	void* data;

	data = stbi_load(path, &self->size.x, &self->size.y, &self->channels, 4);

	if (!data)
	{
		printf(STRING_ERROR_TEXTURE_INIT, path);
		return false;
	}

	printf(STRING_INFO_TEXTURE_INIT, path);

	texture_gl_set(self, data);

	return true;
}

bool
texture_from_data_init(Texture* self, const u8* data, u32 length)
{
	void* textureData;

	textureData = stbi_load_from_memory(data, length, &self->size.x, &self->size.y, &self->channels, 4);

	if (!textureData)
		return false;

	texture_gl_set(self, textureData);

	return true;
}

/* Writes a *.png to the path from the data/size */
/* Returns true on success */
bool
texture_from_data_write(const char* path, const u8* data, s32 width, s32 height)
{
	return (bool)stbi_write_png(path, width, height, 4, data, width * 4);
}

void
texture_free(Texture* self)
{
	glDeleteTextures(1, &self->id);
	memset(self, '\0', sizeof(Texture));
}