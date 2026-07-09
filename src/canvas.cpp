#include "canvas.hpp"

#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "math.hpp"

using namespace glm;
using namespace anm2ed::resource;
using namespace anm2ed::util;

namespace anm2ed
{
  void canvas_blend_premultiplied_set()
  {
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }

  void canvas_blend_straight_set()
  {
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
  }

  Canvas::Canvas() = default;

  Canvas::Canvas(vec2 size)
  {
    Framebuffer::size_set(size);

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
  }

  Canvas::~Canvas()
  {
    if (!Framebuffer::is_valid()) return;

    glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);

    glDeleteVertexArrays(1, &rectVAO);
    glDeleteBuffers(1, &rectVBO);
  }

  mat4 Canvas::transform_get(float zoom, vec2 pan) const
  {
    auto zoomFactor = math::percent_to_unit(zoom);
    auto projection = glm::ortho(0.0f, (float)size.x, 0.0f, (float)size.y, -1.0f, 1.0f);
    auto view = mat4{1.0f};

    view = glm::translate(view, vec3((size * 0.5f) + pan, 0.0f));
    view = glm::scale(view, vec3(zoomFactor, zoomFactor, 1.0f));

    return projection * view;
  }

  void Canvas::axes_render(Shader& shader, float zoom, vec2 pan, vec4 color) const
  {
    canvas_blend_premultiplied_set();

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
    canvas_blend_straight_set();
  }

  void Canvas::grid_render(Shader& shader, float zoom, vec2 pan, ivec2 size, ivec2 offset, vec4 color) const
  {
    canvas_blend_premultiplied_set();

    auto transform = glm::inverse(transform_get(zoom, pan));

    glUseProgram(shader.id);

    glUniformMatrix4fv(glGetUniformLocation(shader.id, shader::UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
    glUniform2f(glGetUniformLocation(shader.id, shader::UNIFORM_SIZE), (float)size.x, (float)size.y);
    glUniform2f(glGetUniformLocation(shader.id, shader::UNIFORM_OFFSET), (float)offset.x, (float)offset.y);
    glUniform4f(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR), color.r, color.g, color.b, color.a);

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glUseProgram(0);
    canvas_blend_straight_set();
  }

  void texture_uniform_vec4_set(const shader::Uniform& uniform, vec4 value)
  {
    if (uniform.location == -1) return;

    switch (uniform.valueType)
    {
      case shader::UNIFORM_VALUE_FLOAT:
        glUniform1f(uniform.location, value.x);
        break;
      case shader::UNIFORM_VALUE_VEC2:
        glUniform2fv(uniform.location, 1, value_ptr(value));
        break;
      case shader::UNIFORM_VALUE_VEC3:
        glUniform3fv(uniform.location, 1, value_ptr(value));
        break;
      case shader::UNIFORM_VALUE_VEC4:
        glUniform4fv(uniform.location, 1, value_ptr(value));
        break;
      default:
        break;
    }
  }

  void texture_uniform_manual_set(const shader::Uniform& uniform)
  {
    if (uniform.location == -1) return;

    switch (uniform.valueType)
    {
      case shader::UNIFORM_VALUE_FLOAT:
        glUniform1f(uniform.location, uniform.value.x);
        break;
      case shader::UNIFORM_VALUE_INT:
      case shader::UNIFORM_VALUE_SAMPLER2D:
        glUniform1i(uniform.location, uniform.intValue);
        break;
      case shader::UNIFORM_VALUE_VEC2:
        glUniform2fv(uniform.location, 1, value_ptr(uniform.value));
        break;
      case shader::UNIFORM_VALUE_VEC3:
        glUniform3fv(uniform.location, 1, value_ptr(uniform.value));
        break;
      case shader::UNIFORM_VALUE_VEC4:
        glUniform4fv(uniform.location, 1, value_ptr(uniform.value));
        break;
      default:
        break;
    }
  }

  float texture_uniform_component_value_get(const shader::Uniform::Component& component, float playbackTime)
  {
    if (component.binding == shader::UNIFORM_BINDING_PLAYBACK_TIME) return playbackTime;
    return component.value;
  }

  vec4 texture_uniform_component_vec4_get(const shader::Uniform& uniform, float playbackTime)
  {
    return {texture_uniform_component_value_get(uniform.components[0], playbackTime),
            texture_uniform_component_value_get(uniform.components[1], playbackTime),
            texture_uniform_component_value_get(uniform.components[2], playbackTime),
            texture_uniform_component_value_get(uniform.components[3], playbackTime)};
  }

  void texture_uniforms_set(Shader& shader, mat4 transform, vec4 tint, vec3 colorOffset, vec2 textureSize,
                            float playbackTime)
  {
    if (shader.uniforms.empty())
    {
      glUniform1i(glGetUniformLocation(shader.id, shader::UNIFORM_TEXTURE), 0);
      glUniform3fv(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR_OFFSET), 1, value_ptr(colorOffset));
      glUniform4fv(glGetUniformLocation(shader.id, shader::UNIFORM_TINT), 1, value_ptr(tint));
      glUniformMatrix4fv(glGetUniformLocation(shader.id, shader::UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
      return;
    }

    for (const auto& uniform : shader.uniforms)
    {
      if (uniform.location == -1) continue;

      switch (uniform.binding)
      {
        case shader::UNIFORM_BINDING_MAIN_TEXTURE:
          glUniform1i(uniform.location, 0);
          break;
        case shader::UNIFORM_BINDING_TRANSFORM:
          if (uniform.valueType == shader::UNIFORM_VALUE_MAT4)
            glUniformMatrix4fv(uniform.location, 1, GL_FALSE, value_ptr(transform));
          break;
        case shader::UNIFORM_BINDING_FRAME_TINT:
          texture_uniform_vec4_set(uniform, tint);
          break;
        case shader::UNIFORM_BINDING_COLOR_OFFSET:
          texture_uniform_vec4_set(uniform, vec4(colorOffset, 0.0f));
          break;
        case shader::UNIFORM_BINDING_TEXTURE_SIZE:
          texture_uniform_vec4_set(uniform, vec4(textureSize, 0.0f, 0.0f));
          break;
        case shader::UNIFORM_BINDING_PLAYBACK_TIME:
          texture_uniform_vec4_set(uniform, vec4(playbackTime));
          break;
        case shader::UNIFORM_BINDING_COMPONENTS:
          texture_uniform_vec4_set(uniform, texture_uniform_component_vec4_get(uniform, playbackTime));
          break;
        case shader::UNIFORM_BINDING_MANUAL:
          texture_uniform_manual_set(uniform);
          break;
        default:
          break;
      }
    }
  }

  void Canvas::texture_render(Shader& shader, GLuint& texture, mat4 transform, vec4 tint, vec3 colorOffset,
                              float* vertices, vec2 textureSize, float playbackTime) const
  {
    canvas_blend_premultiplied_set();

    glUseProgram(shader.id);

    texture_uniforms_set(shader, transform, tint, colorOffset, textureSize, playbackTime);
    glVertexAttrib4fv(2, value_ptr(tint));

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

    canvas_blend_straight_set();
  }

  void Canvas::rect_render(Shader& shader, const mat4& transform, const mat4& model, vec4 color, float dashLength,
                           float dashGap, float dashOffset) const
  {
    canvas_blend_premultiplied_set();

    glUseProgram(shader.id);

    glUniformMatrix4fv(glGetUniformLocation(shader.id, shader::UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
    if (auto location = glGetUniformLocation(shader.id, shader::UNIFORM_MODEL); location != -1)
      glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(model));
    glUniform4fv(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR), 1, value_ptr(color));

    auto origin = model * vec4(0.0f, 0.0f, 0.0f, 1.0f);
    auto edgeX = model * vec4(1.0f, 0.0f, 0.0f, 1.0f);
    auto edgeY = model * vec4(0.0f, 1.0f, 0.0f, 1.0f);

    auto axisX = vec2(edgeX - origin);
    auto axisY = vec2(edgeY - origin);

    if (auto location = glGetUniformLocation(shader.id, shader::UNIFORM_AXIS_X); location != -1)
      glUniform2fv(location, 1, value_ptr(axisX));
    if (auto location = glGetUniformLocation(shader.id, shader::UNIFORM_AXIS_Y); location != -1)
      glUniform2fv(location, 1, value_ptr(axisY));

    if (auto location = glGetUniformLocation(shader.id, shader::UNIFORM_DASH_LENGTH); location != -1)
      glUniform1f(location, dashLength);
    if (auto location = glGetUniformLocation(shader.id, shader::UNIFORM_DASH_GAP); location != -1)
      glUniform1f(location, dashGap);
    if (auto location = glGetUniformLocation(shader.id, shader::UNIFORM_DASH_OFFSET); location != -1)
      glUniform1f(location, dashOffset);

    glBindVertexArray(rectVAO);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
    canvas_blend_straight_set();
  }

  void Canvas::rect_fill_render(Shader& shader, const mat4& transform, const mat4& model, vec4 color) const
  {
    canvas_blend_premultiplied_set();

    glUseProgram(shader.id);

    glUniformMatrix4fv(glGetUniformLocation(shader.id, shader::UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
    if (auto location = glGetUniformLocation(shader.id, shader::UNIFORM_MODEL); location != -1)
      glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(model));
    glUniform4fv(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR), 1, value_ptr(color));

    glBindVertexArray(rectVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
    canvas_blend_straight_set();
  }

  float Canvas::zoom_level_get(float zoom, int levelDelta) const
  {
    if (levelDelta == 0) return zoom;
    if (zoom < ZOOM_LEVELS.front()) return ZOOM_LEVELS.front();
    if (zoom > ZOOM_LEVELS.back()) return ZOOM_LEVELS.back();

    auto levelCount = (int)ZOOM_LEVELS.size();
    int levelIndex{};
    if (levelDelta > 0)
    {
      auto it = std::upper_bound(ZOOM_LEVELS.begin(), ZOOM_LEVELS.end(), zoom);
      levelIndex = (int)std::distance(ZOOM_LEVELS.begin(), it) + levelDelta - 1;
    }
    else
    {
      auto it = std::lower_bound(ZOOM_LEVELS.begin(), ZOOM_LEVELS.end(), zoom);
      levelIndex = (int)std::distance(ZOOM_LEVELS.begin(), it) + levelDelta;
    }

    return ZOOM_LEVELS[std::clamp(levelIndex, 0, levelCount - 1)];
  }

  void Canvas::zoom_level_adjust(float& zoom, vec2& pan, vec2 focus, int levelDelta) const
  {
    auto zoomFactor = math::percent_to_unit(zoom);
    float newZoom = zoom_level_get(zoom, levelDelta);
    if (newZoom != zoom)
    {
      float newZoomFactor = math::percent_to_unit(newZoom);
      pan += focus * (zoomFactor - newZoomFactor);
      zoom = newZoom;
    }
  }

  vec2 Canvas::position_translate(float& zoom, vec2& pan, vec2 position) const
  {
    auto zoomFactor = math::percent_to_unit(zoom);
    return (position - pan - (size * 0.5f)) / zoomFactor;
  }

  void Canvas::set_to_rect(float& zoom, vec2& pan, vec4 rect) const
  {
    if (rect != vec4(-1.0f) && (rect.z > 0 && rect.w > 0))
    {
      f32 scaleX = size.x / rect.z;
      f32 scaleY = size.y / rect.w;
      f32 fitScale = std::min(scaleX, scaleY);

      zoom = math::unit_to_percent(fitScale);

      vec2 rectCenter = {rect.x + rect.z * 0.5f, rect.y + rect.w * 0.5f};
      pan = -rectCenter * fitScale;
    }
  }
}
