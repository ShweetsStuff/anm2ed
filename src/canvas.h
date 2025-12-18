#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "framebuffer.h"
#include "shader.h"

#include "types.h"

namespace anm2ed
{
  class Canvas : public Framebuffer
  {
  public:
    static constexpr float TEXTURE_VERTICES[] = {0, 0, 0.0f, 0.0f, 1, 0, 1.0f, 0.0f,
                                                 1, 1, 1.0f, 1.0f, 0, 1, 0.0f, 1.0f};
    static constexpr auto PIVOT_SIZE = glm::vec2(8, 8);
    static constexpr auto ZOOM_MIN = 1.0f;
    static constexpr auto ZOOM_MAX = 2000.0f;
    static constexpr auto DASH_LENGTH = 4.0f;
    static constexpr auto DASH_GAP = 1.0f;
    static constexpr auto DASH_OFFSET = 1.0f;
    static constexpr auto STEP = 1.0f;
    static constexpr auto STEP_FAST = 5.0f;
    static constexpr auto GRID_SIZE_MIN = 1;
    static constexpr auto GRID_SIZE_MAX = 10000;
    static constexpr auto GRID_OFFSET_MIN = 0;
    static constexpr auto GRID_OFFSET_MAX = 10000;
    static constexpr auto CHECKER_SIZE = 32.0f;
    static constexpr float AXIS_VERTICES[] = {-1.0f, 0.0f, 1.0f, 0.0f};
    static constexpr float GRID_VERTICES[] = {-1.f, -1.f, 0.f, 0.f, 3.f, -1.f, 2.f, 0.f, -1.f, 3.f, 0.f, 2.f};
    static constexpr float RECT_VERTICES[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
    static constexpr GLuint TEXTURE_INDICES[] = {0, 1, 2, 2, 3, 0};
    static constexpr auto BORDER_DASH_LENGTH = 1.0f;
    static constexpr auto BORDER_DASH_GAP = 0.5f;
    static constexpr auto BORDER_DASH_OFFSET = 0.0f;
    static constexpr auto PIVOT_COLOR = types::color::PINK;

    GLuint axisVAO{};
    GLuint axisVBO{};
    GLuint rectVAO{};
    GLuint rectVBO{};
    GLuint gridVAO{};
    GLuint gridVBO{};
    GLuint textureVAO{};
    GLuint textureVBO{};
    GLuint textureEBO{};

    Canvas();
    Canvas(glm::vec2);
    ~Canvas();
    glm::mat4 transform_get(float = 100.0f, glm::vec2 = {}) const;
    void axes_render(resource::Shader&, float, glm::vec2, glm::vec4 = glm::vec4(1.0f)) const;
    void grid_render(resource::Shader&, float, glm::vec2, glm::ivec2 = glm::ivec2(32, 32), glm::ivec2 = {},
                     glm::vec4 = glm::vec4(1.0f)) const;
    void texture_render(resource::Shader&, GLuint&, glm::mat4 = {1.0f}, glm::vec4 = glm::vec4(1.0f), glm::vec3 = {},
                        float* = (float*)TEXTURE_VERTICES) const;
    void rect_render(resource::Shader&, const glm::mat4&, const glm::mat4&, glm::vec4 = glm::vec4(1.0f),
                     float dashLength = DASH_LENGTH, float dashGap = DASH_GAP, float dashOffset = DASH_OFFSET) const;
    void zoom_set(float&, glm::vec2&, glm::vec2, float) const;
    glm::vec2 position_translate(float&, glm::vec2&, glm::vec2) const;
    void set_to_rect(float& zoom, glm::vec2& pan, glm::vec4 rect) const;
  };
}
