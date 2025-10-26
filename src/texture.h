#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace anm2ed::texture
{
  constexpr auto CHANNELS = 4;

  class Texture
  {
  public:
    GLuint id{};
    glm::ivec2 size{};
    GLint filter = GL_NEAREST;
    int channels{};
    std::vector<uint8_t> pixels{};

    bool is_valid();
    void download();
    void upload(const uint8_t*);
    void upload();
    Texture();

    ~Texture();
    Texture(const Texture&);
    Texture(Texture&&);
    Texture& operator=(const Texture&);
    Texture& operator=(Texture&&);
    Texture(const char*, size_t, glm::ivec2);
    Texture(const std::string&);
    bool write_png(const std::string&);
    void pixel_set(glm::ivec2, glm::vec4);
    void pixel_line(glm::ivec2, glm::ivec2, glm::vec4);
    void bind(GLuint = 0);
    void unbind(GLuint = 0);
  };
}
