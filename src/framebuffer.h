#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace anm2ed
{
  class Framebuffer
  {
  public:
    enum Type
    {
      UNDERLAY,
      LAYER,
      OVERLAY,
    };

    GLuint fbo{};
    GLuint rbo{};
    GLuint texture{};
    glm::vec2 size{};
    glm::vec2 previousSize{};

    Framebuffer();
    ~Framebuffer();

    void set();
    void resize_check();
    void size_set(glm::vec2);
    void viewport_set() const;
    void clear(glm::vec4 = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)) const;
    std::vector<uint8_t> pixels_get() const;
    bool is_valid() const;
    void bind() const;
    void unbind() const;
  };
};