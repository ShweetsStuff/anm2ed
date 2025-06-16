#include "texture.h"

#define STBI_ONLY_PNG  
#define STBI_NO_FAILURE_STRINGS   
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool
texture_init(Texture* self, const char* path)
{
	void* data;

	data = stbi_load(path, &self->size[0], &self->size[1], &self->channels, 4);

	if (!data)
		return false;

	glGenTextures(1, &self->handle);
	
	glBindTexture(GL_TEXTURE_2D, self->handle);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->size.x, self->size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

void
texture_free(Texture* self)
{
	glDeleteTextures(1, &self->handle);
	memset(self, '\0', sizeof(Texture));
}
