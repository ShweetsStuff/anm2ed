#include "texture.h"

#define STBI_ONLY_PNG  
#define STBI_NO_FAILURE_STRINGS   
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// Generates GL texture and sets parameters
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

// Initializes texture from path; returns true/false on success
bool
texture_from_path_init(Texture* self, const std::string& path)
{
	void* data;

	data = stbi_load(path.c_str(), &self->size.x, &self->size.y, &self->channels, 4);

	if (!data)
	{
		std::cout << STRING_ERROR_TEXTURE_INIT << path << std::endl;
		return false;
	}

	std::cout << STRING_INFO_TEXTURE_INIT << path << std::endl;

	texture_gl_set(self, data);

	return true;
}

// Initializes texture from data; returns true/false on success
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

// Writes an image to the path from the data/size
bool
texture_from_data_write(const std::string& path, const u8* data, s32 width, s32 height)
{
	return (bool)stbi_write_png(path.c_str(), width, height, 4, data, width * 4);
}

// Frees texture
void
texture_free(Texture* self)
{
	glDeleteTextures(1, &self->id);
	*self = Texture{};
}