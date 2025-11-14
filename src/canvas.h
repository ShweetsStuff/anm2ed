#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "shader.h"

namespace anm2ed::canvas
{
  constexpr float TEXTURE_VERTICES[] = {0, 0, 0.0f, 0.0f, 1, 0, 1.0f, 0.0f, 1, 1, 1.0f, 1.0f, 0, 1, 0.0f, 1.0f};

  constexpr auto PIVOT_SIZE = glm::vec2(8, 8);
  constexpr auto ZOOM_MIN = 1.0f;
  constexpr auto ZOOM_MAX = 2000.0f;
  constexpr auto POSITION_FORMAT = "Position: ({:8}, {:8})";

  constexpr auto DASH_LENGTH = 4.0f;
  constexpr auto DASH_GAP = 1.0f;
  constexpr auto DASH_OFFSET = 1.0f;

  constexpr auto STEP = 1.0f;
  constexpr auto STEP_FAST = 5.0f;

  constexpr auto GRID_SIZE_MIN = 1;
  constexpr auto GRID_SIZE_MAX = 10000;
  constexpr auto GRID_OFFSET_MIN = 0;
  constexpr auto GRID_OFFSET_MAX = 10000;

  constexpr auto CHECKER_SIZE = 32.0f;
}

namespace anm2ed
{
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
    bool is_valid() const;
    void framebuffer_set() const;
    void framebuffer_resize_check();
    void size_set(glm::vec2);
    glm::vec4 pixel_read(glm::vec2, glm::vec2) const;
    glm::mat4 transform_get(float = 100.0f, glm::vec2 = {}) const;
    void axes_render(resource::Shader&, float, glm::vec2, glm::vec4 = glm::vec4(1.0f)) const;
    void grid_render(resource::Shader&, float, glm::vec2, glm::ivec2 = glm::ivec2(32, 32), glm::ivec2 = {},
                     glm::vec4 = glm::vec4(1.0f)) const;
    void texture_render(resource::Shader&, GLuint&, glm::mat4&, glm::vec4 = glm::vec4(1.0f), glm::vec3 = {},
                        float* = (float*)canvas::TEXTURE_VERTICES) const;
    void rect_render(resource::Shader&, const glm::mat4&, const glm::mat4&, glm::vec4 = glm::vec4(1.0f),
                     float dashLength = canvas::DASH_LENGTH, float dashGap = canvas::DASH_GAP,
                     float dashOffset = canvas::DASH_OFFSET) const;
    void viewport_set() const;
    void clear(glm::vec4 = glm::vec4()) const;
    void bind() const;
    void unbind() const;
    void zoom_set(float&, glm::vec2&, glm::vec2, float) const;
    glm::vec2 position_translate(float&, glm::vec2&, glm::vec2) const;
    void set_to_rect(float& zoom, glm::vec2& pan, glm::vec4 rect) const;
    std::vector<unsigned char> pixels_get() const;
  };
}
