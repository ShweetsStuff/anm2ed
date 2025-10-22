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
    void download(std::vector<uint8_t>& pixels);
    void init(const uint8_t* data, bool isDownload = false);
    Texture();

    ~Texture();
    Texture(Texture&& other);
    Texture& operator=(Texture&& other);
    Texture(const char* svgData, size_t svgDataLength, glm::ivec2 svgSize);
    Texture(const std::string& pngPath, bool isDownload = false);
    bool write_png(const std::string& path);
    void bind(GLuint unit = 0);
    void unbind(GLuint unit = 0);
  };
}