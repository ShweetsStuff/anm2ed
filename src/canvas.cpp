#include "canvas.h"

#include "math.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace anm2ed::shader;

namespace anm2ed::canvas
{
  constexpr float AXIS_VERTICES[] = {-1.0f, 0.0f, 1.0f, 0.0f};
  constexpr float GRID_VERTICES[] = {-1.f, -1.f, 0.f, 0.f, 3.f, -1.f, 2.f, 0.f, -1.f, 3.f, 0.f, 2.f};
  constexpr float RECT_VERTICES[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
  constexpr GLuint TEXTURE_INDICES[] = {0, 1, 2, 2, 3, 0};

  Canvas::Canvas() = default;

  Canvas::Canvas(vec2 size)
  {
    this->size = size;
    previousSize = size;

    // Axis
    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);

    glBindVertexArray(axisVAO);

    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(AXIS_VERTICES), AXIS_VERTICES, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Grid
    glGenVertexArrays(1, &gridVAO);
    glBindVertexArray(gridVAO);

    glGenBuffers(1, &gridVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GRID_VERTICES), GRID_VERTICES, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

    glBindVertexArray(0);

    // Rect
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);

    glBindVertexArray(rectVAO);

    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RECT_VERTICES), RECT_VERTICES, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Texture
    glGenVertexArrays(1, &textureVAO);
    glGenBuffers(1, &textureVBO);
    glGenBuffers(1, &textureEBO);

    glBindVertexArray(textureVAO);

    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(TEXTURE_INDICES), TEXTURE_INDICES, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    // Framebuffer
    glGenTextures(1, &texture);
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);

    framebuffer_set();
  }

  Canvas::~Canvas()
  {
    if (!is_valid()) return;

    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &texture);

    glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);

    glDeleteVertexArrays(1, &rectVAO);
    glDeleteBuffers(1, &rectVBO);
  }

  bool Canvas::is_valid()
  {
    return fbo != 0;
  }

  void Canvas::framebuffer_set()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void Canvas::framebuffer_resize_check()
  {
    if (previousSize != size)
    {
      framebuffer_set();
      previousSize = size;
    }
  }

  void Canvas::size_set(vec2 size)
  {
    this->size = size;
    framebuffer_resize_check();
  }

  mat4 Canvas::transform_get(float zoom, vec2 pan)
  {
    auto zoomFactor = math::percent_to_unit(zoom);
    auto projection = glm::ortho(0.0f, (float)size.x, 0.0f, (float)size.y, -1.0f, 1.0f);
    auto view = mat4{1.0f};

    view = glm::translate(view, vec3((size * 0.5f) + pan, 0.0f));
    view = glm::scale(view, vec3(zoomFactor, zoomFactor, 1.0f));

    return projection * view;
  }

  void Canvas::axes_render(Shader& shader, float zoom, vec2 pan, vec4 color)
  {
    auto originNDC = transform_get(zoom, pan) * vec4(0.0f, 0.0f, 0.0f, 1.0f);
    originNDC /= originNDC.w;

    glUseProgram(shader.id);

    glUniform4fv(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR), 1, value_ptr(color));
    glUniform2f(glGetUniformLocation(shader.id, shader::UNIFORM_ORIGIN_NDC), originNDC.x, originNDC.y);

    glBindVertexArray(axisVAO);

    glUniform1i(glGetUniformLocation(shader.id, shader::UNIFORM_AXIS), 0);
    glDrawArrays(GL_LINES, 0, 2);

    glUniform1i(glGetUniformLocation(shader.id, shader::UNIFORM_AXIS), 1);
    glDrawArrays(GL_LINES, 0, 2);

    glBindVertexArray(0);
    glUseProgram(0);
  }

  void Canvas::grid_render(Shader& shader, float zoom, vec2 pan, ivec2 size, ivec2 offset, vec4 color)
  {
    auto zoomFactor = math::percent_to_unit(zoom);

    glUseProgram(shader.id);

    glUniform2f(glGetUniformLocation(shader.id, shader::UNIFORM_VIEW_SIZE), this->size.x, this->size.y);
    glUniform2f(glGetUniformLocation(shader.id, shader::UNIFORM_PAN), pan.x, pan.y);
    glUniform1f(glGetUniformLocation(shader.id, shader::UNIFORM_ZOOM), zoomFactor);
    glUniform2f(glGetUniformLocation(shader.id, shader::UNIFORM_SIZE), (float)size.x, (float)size.y);
    glUniform2f(glGetUniformLocation(shader.id, shader::UNIFORM_OFFSET), (float)offset.x, (float)offset.y);
    glUniform4f(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR), color.r, color.g, color.b, color.a);

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glUseProgram(0);
  }

  void Canvas::texture_render(Shader& shader, GLuint& texture, mat4& transform, vec4 tint, vec3 colorOffset,
                              float* vertices)
  {
    glUseProgram(shader.id);

    glUniform1i(glGetUniformLocation(shader.id, shader::UNIFORM_TEXTURE), 0);
    glUniform3fv(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR_OFFSET), 1, value_ptr(colorOffset));
    glUniform4fv(glGetUniformLocation(shader.id, shader::UNIFORM_TINT), 1, value_ptr(tint));
    glUniformMatrix4fv(glGetUniformLocation(shader.id, shader::UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));

    glBindVertexArray(textureVAO);

    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TEXTURE_VERTICES), vertices, GL_DYNAMIC_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
  }

  void Canvas::rect_render(Shader& shader, mat4& transform, vec4 color)
  {
    glUseProgram(shader.id);

    glUniformMatrix4fv(glGetUniformLocation(shader.id, shader::UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
    glUniform4fv(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR), 1, value_ptr(color));

    glBindVertexArray(rectVAO);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
  }

  void Canvas::viewport_set()
  {
    glViewport(0, 0, size.x, size.y);
  }

  void Canvas::clear(vec4& color)
  {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void Canvas::bind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  }

  void Canvas::unbind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void Canvas::zoom_set(float& zoom, vec2& pan, vec2& focus, float step)
  {
    auto zoomFactor = math::percent_to_unit(zoom);
    float newZoom = glm::clamp(math::round_nearest_multiple(zoom + step, step), canvas::ZOOM_MIN, canvas::ZOOM_MAX);
    if (newZoom != zoom)
    {
      float newZoomFactor = math::percent_to_unit(newZoom);
      pan += focus * (zoomFactor - newZoomFactor);
      zoom = newZoom;
    }
  }

  vec2 Canvas::position_translate(float& zoom, vec2& pan, vec2 position)
  {
    auto zoomFactor = math::percent_to_unit(zoom);
    return (position - pan - (size * 0.5f)) / zoomFactor;
  }
}
