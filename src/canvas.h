#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "shader.h"

namespace anm2ed::canvas
{
  constexpr float TEXTURE_VERTICES[] = {0, 0, 0.0f, 0.0f, 1, 0, 1.0f, 0.0f, 1, 1, 1.0f, 1.0f, 0, 1, 0.0f, 1.0f};

  constexpr auto ZOOM_MIN = 1.0f;
  constexpr auto ZOOM_MAX = 2000.0f;
  constexpr auto POSITION_FORMAT = "Position: ({:8} {:8})";

  class Canvas
  {
  public:
    GLuint fbo{};
    GLuint rbo{};
    GLuint axisVAO{};
    GLuint axisVBO{};
    GLuint rectVAO{};
    GLuint rectVBO{};
    GLuint gridVAO{};
    GLuint gridVBO{};
    GLuint textureVAO{};
    GLuint textureVBO{};
    GLuint textureEBO{};
    GLuint texture{};
    glm::vec2 previousSize{};
    glm::vec2 size{};

    Canvas();
    Canvas(glm::vec2);
    ~Canvas();
    bool is_valid();
    void framebuffer_set();
    void framebuffer_resize_check();
    void size_set(glm::vec2);
    glm::mat4 transform_get(float, glm::vec2);
    void axes_render(shader::Shader&, float, glm::vec2, glm::vec4 = glm::vec4(1.0f));
    void grid_render(shader::Shader&, float, glm::vec2, glm::ivec2 = glm::ivec2(32, 32), glm::ivec2 = {},
                     glm::vec4 = glm::vec4(1.0f));
    void texture_render(shader::Shader&, GLuint&, glm::mat4&, glm::vec4 = glm::vec4(1.0f), glm::vec3 = {},
                        float* = (float*)TEXTURE_VERTICES);
    void rect_render(shader::Shader&, glm::mat4&, glm::vec4 = glm::vec4(1.0f));
    void viewport_set();
    void clear(glm::vec4&);
    void bind();
    void unbind();
    void zoom_set(float&, glm::vec2&, glm::vec2&, float);
    glm::vec2 position_translate(float&, glm::vec2&, glm::vec2);
  };
}
