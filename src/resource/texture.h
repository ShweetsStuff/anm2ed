#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace anm2ed::resource::texture
{
  constexpr auto CHANNELS = 4;
}

namespace anm2ed::resource
{
  class Texture
  {
  public:
    GLuint id{};
    glm::ivec2 size{};
    GLint filter = GL_NEAREST;
    int channels{};
    std::vector<uint8_t> pixels{};

    bool is_valid();
    size_t pixel_size_get();
    void upload();
    void upload(const uint8_t*);

    Texture();
    ~Texture();
    Texture(const Texture&);
    Texture(Texture&&);
    Texture& operator=(const Texture&);
    Texture& operator=(Texture&&);
    Texture(const uint8_t*, glm::ivec2);
    Texture(const char*, size_t, glm::ivec2);
    Texture(const std::string&);
    bool write_png(const std::string&);
    void pixel_set(glm::ivec2, glm::vec4);
    void pixel_line(glm::ivec2, glm::ivec2, glm::vec4);
  };
}
