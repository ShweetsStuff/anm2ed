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
    Canvas(glm::vec2 size);
    ~Canvas();
    bool is_valid();
    void framebuffer_set();
    void framebuffer_resize_check();
    void size_set(glm::vec2 size);
    glm::mat4 transform_get(float zoom, glm::vec2 pan);
    void axes_render(shader::Shader& shader, float zoom, glm::vec2 pan, glm::vec4 color = glm::vec4(1.0f));
    void grid_render(shader::Shader& shader, float zoom, glm::vec2 pan, glm::ivec2 size = glm::ivec2(32, 32),
                     glm::ivec2 offset = {}, glm::vec4 color = glm::vec4(1.0f));
    void texture_render(shader::Shader& shader, GLuint& texture, glm::mat4& transform, glm::vec4 tint = glm::vec4(1.0f),
                        glm::vec3 colorOffset = {}, float* vertices = (float*)TEXTURE_VERTICES);
    void rect_render(shader::Shader& shader, glm::mat4& transform, glm::vec4 color = glm::vec4(1.0f));
    void viewport_set();
    void clear(glm::vec4& color);
    void bind();
    void unbind();
    void zoom_set(float& zoom, glm::vec2& pan, glm::vec2& focus, float step);
    glm::vec2 position_translate(float& zoom, glm::vec2& pan, glm::vec2 position);
  };
}
