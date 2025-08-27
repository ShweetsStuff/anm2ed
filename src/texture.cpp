#if defined(__clang__) || defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  #pragma GCC diagnostic ignored "-Wunused-function"
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include "texture.h"

#define STBI_ONLY_PNG  
#define STBI_NO_FAILURE_STRINGS   
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

static void _texture_gl_set(Texture* self, const u8* data)
{
	if (self->id == GL_ID_NONE) 
		glGenTextures(1, &self->id);
	
	glBindTexture(GL_TEXTURE_2D, self->id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->size.x, self->size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0); 
}

std::vector<u8> texture_download(const Texture* self)
{
    std::vector<u8> pixels(self->size.x * self->size.y * TEXTURE_CHANNELS);
    
	glBindTexture(GL_TEXTURE_2D, self->id);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
 
	return pixels;
}

bool texture_from_path_init(Texture* self, const std::string& path)
{
	u8* data = stbi_load(path.c_str(), &self->size.x, &self->size.y, &self->channels, TEXTURE_CHANNELS);
	
	if (!data)
	{
		data = stbi_load(path_canonical_resolve(path).c_str(), &self->size.x, &self->size.y, &self->channels, TEXTURE_CHANNELS);
		if (!data)
		{
			log_error(std::format(TEXTURE_INIT_ERROR, path));
			return false;
		}
	}

	self->isInvalid = false;

	log_info(std::format(TEXTURE_INIT_INFO, path));

	_texture_gl_set(self, data);

	return true;
}

bool texture_from_encoded_data_init(Texture* self, ivec2 size, s32 channels, const u8* data, u32 length)
{
	*self = Texture{};
	self->size = size;
	self->channels = channels;

	u8* textureData = stbi_load_from_memory(data, length, &self->size.x, &self->size.y, &self->channels, TEXTURE_CHANNELS);

	if (!textureData) 
	{
		self->isInvalid = true;
		return false;
	}

	_texture_gl_set(self, textureData);

	return true;
}

bool texture_from_rgba_init(Texture* self, ivec2 size, s32 channels, const u8* data)
{
	*self = Texture{};
	self->size = size;
	self->channels = channels;

	_texture_gl_set(self, data);

	return true;
}

bool texture_from_rgba_write(const std::string& path, const u8* data, ivec2 size)
{
	bool isSuccess = stbi_write_png(path.c_str(), size.x, size.y, TEXTURE_CHANNELS, data, size.x * TEXTURE_CHANNELS);
	if (!isSuccess)
	{
		isSuccess = stbi_write_png(path_canonical_resolve(path).c_str(), size.x, size.y, TEXTURE_CHANNELS, data, size.x * TEXTURE_CHANNELS);
		if (!isSuccess) log_info(std::format(TEXTURE_SAVE_ERROR, path));
	}
		
	log_info(std::format(TEXTURE_SAVE_INFO, path));
		
	return isSuccess;
}

bool texture_from_gl_write(Texture* self, const std::string& path)
{
	return texture_from_rgba_write(path, texture_download(self).data(), self->size);
}

void texture_free(Texture* self)
{
	glDeleteTextures(1, &self->id);
	*self = Texture{};
}

bool texture_pixel_set(Texture* self, ivec2 position, vec4 color)
{
    if 
	(
		position.x < 0 || position.y < 0 || 
		position.x >= self->size.x || position.y >= self->size.y
	)
        return false;

    uint8_t rgba8[4] = {FLOAT_TO_U8(color.r), FLOAT_TO_U8(color.g), FLOAT_TO_U8(color.b), FLOAT_TO_U8(color.a)};

    glBindTexture(GL_TEXTURE_2D, self->id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
    glTexSubImage2D(GL_TEXTURE_2D, 0,position.x, position.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba8);

    return true;
}