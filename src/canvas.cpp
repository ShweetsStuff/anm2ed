#include "canvas.hpp"

#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "math.hpp"
#include "texture.hpp"

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

  void vertex_array_make(GLuint& vao, GLuint& vbo, const void* data, GLsizeiptr size, GLenum usage,
                         std::initializer_list<int> attributeSizes)
  {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);

    auto stride = 0;
    for (auto attributeSize : attributeSizes)
      stride += attributeSize;
    auto offset = 0;
    auto index = 0;
    for (auto attributeSize : attributeSizes)
    {
      glEnableVertexAttribArray(index);
      glVertexAttribPointer(index++, attributeSize, GL_FLOAT, GL_FALSE, stride * sizeof(float),
                            (void*)(offset * sizeof(float)));
      offset += attributeSize;
    }
  }

  Canvas::Canvas() = default;

  Canvas::Canvas(vec2 size)
  {
    Framebuffer::size_set(size);
    vertex_array_make(axisVAO, axisVBO, AXIS_VERTICES, sizeof(AXIS_VERTICES), GL_STATIC_DRAW, {2});
    vertex_array_make(gridVAO, gridVBO, GRID_VERTICES, sizeof(GRID_VERTICES), GL_STATIC_DRAW, {2, 2});
    vertex_array_make(rectVAO, rectVBO, RECT_VERTICES, sizeof(RECT_VERTICES), GL_STATIC_DRAW, {2});
    vertex_array_make(textureVAO, textureVBO, nullptr, sizeof(TEXTURE_VERTICES), GL_DYNAMIC_DRAW, {2, 2});
    glGenBuffers(1, &textureEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(TEXTURE_INDICES), TEXTURE_INDICES, GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
  }

  Canvas::~Canvas()
  {
    if (!Framebuffer::is_valid()) return;
    for (auto vao : {axisVAO, gridVAO, rectVAO, textureVAO})
      glDeleteVertexArrays(1, &vao);
    for (auto buffer : {axisVBO, gridVBO, rectVBO, textureVBO, textureEBO})
      glDeleteBuffers(1, &buffer);
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

  void texture_uniform_set(const shader::Uniform& uniform, vec4 value)
  {
    if (uniform.valueType == shader::UNIFORM_VALUE_INT || uniform.valueType == shader::UNIFORM_VALUE_SAMPLER2D)
      return glUniform1i(uniform.location, uniform.intValue);
    switch (shader::UNIFORM_VALUE_TYPE_INFOS[uniform.valueType].componentCount)
    {
      case 2:
        return glUniform2fv(uniform.location, 1, value_ptr(value));
      case 3:
        return glUniform3fv(uniform.location, 1, value_ptr(value));
      case 4:
        return glUniform4fv(uniform.location, 1, value_ptr(value));
      default:
        if (uniform.valueType == shader::UNIFORM_VALUE_FLOAT) glUniform1f(uniform.location, value.x);
    }
  }

  vec4 texture_uniform_components_get(const shader::Uniform& uniform, float playbackTime)
  {
    vec4 result{};
    for (int i = 0; i < (int)uniform.components.size(); ++i)
      result[i] = uniform.components[i].binding == shader::UNIFORM_BINDING_PLAYBACK_TIME ? playbackTime
                                                                                         : uniform.components[i].value;
    return result;
  }

  void texture_uniforms_set(Shader& shader, mat4 transform, vec4 tint, vec4 colorOffset, vec2 textureSize,
                            float playbackTime)
  {
    if (shader.uniforms.empty())
    {
      glUniform1i(glGetUniformLocation(shader.id, shader::UNIFORM_TEXTURE), 0);
      glUniform4fv(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR_OFFSET), 1, value_ptr(colorOffset));
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
          texture_uniform_set(uniform, tint);
          break;
        case shader::UNIFORM_BINDING_COLOR_OFFSET:
          texture_uniform_set(uniform, colorOffset);
          break;
        case shader::UNIFORM_BINDING_TEXTURE_SIZE:
          texture_uniform_set(uniform, vec4(textureSize, 0.0f, 0.0f));
          break;
        case shader::UNIFORM_BINDING_PLAYBACK_TIME:
          texture_uniform_set(uniform, vec4(playbackTime));
          break;
        case shader::UNIFORM_BINDING_COMPONENTS:
          texture_uniform_set(uniform, texture_uniform_components_get(uniform, playbackTime));
          break;
        case shader::UNIFORM_BINDING_MANUAL:
          texture_uniform_set(uniform, uniform.value);
          break;
        default:
          break;
      }
    }
  }

  void Canvas::texture_render(Shader& shader, GLuint texture, mat4 transform, vec4 tint, vec4 colorOffset,
                              float* vertices, vec2 textureSize, float playbackTime, TextureBlur blur) const
  {
    canvas_blend_premultiplied_set();

    glUseProgram(shader.id);

    texture_uniforms_set(shader, transform, tint, colorOffset, textureSize, playbackTime);
    // The editor's own texture shader blurs; a custom shader without these uniforms ignores them.
    glUniform1f(glGetUniformLocation(shader.id, shader::UNIFORM_BLUR), blur.radius);
    glUniform4fv(glGetUniformLocation(shader.id, shader::UNIFORM_REGION), 1, value_ptr(blur.region));
    glUniform2fv(glGetUniformLocation(shader.id, shader::UNIFORM_TEXEL), 1,
                 value_ptr(textureSize.x > 0 && textureSize.y > 0 ? 1.0f / textureSize : vec2()));
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

  void rect_begin(Shader& shader, const mat4& transform, const mat4& model, vec4 color)
  {
    canvas_blend_premultiplied_set();
    glUseProgram(shader.id);
    glUniformMatrix4fv(glGetUniformLocation(shader.id, shader::UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
    if (auto location = glGetUniformLocation(shader.id, shader::UNIFORM_MODEL); location != -1)
      glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(model));
    glUniform4fv(glGetUniformLocation(shader.id, shader::UNIFORM_COLOR), 1, value_ptr(color));
  }

  void rect_end(GLuint vao, GLenum mode, int count = 4)
  {
    glBindVertexArray(vao);
    glDrawArrays(mode, 0, count);
    glBindVertexArray(0);
    glUseProgram(0);
    canvas_blend_straight_set();
  }

  void Canvas::dashed_render(Shader& shader, const mat4& transform, const mat4& model, vec4 color, float dashLength,
                             float dashGap, float dashOffset, GLenum mode, int count) const
  {
    rect_begin(shader, transform, model, color);

    auto origin = model * vec4(0.0f, 0.0f, 0.0f, 1.0f);
    for (auto [name, axis] : {std::pair{shader::UNIFORM_AXIS_X, vec4(1.0f, 0.0f, 0.0f, 1.0f)},
                              std::pair{shader::UNIFORM_AXIS_Y, vec4(0.0f, 1.0f, 0.0f, 1.0f)}})
      if (auto location = glGetUniformLocation(shader.id, name); location != -1)
        glUniform2fv(location, 1, value_ptr(vec2(model * axis - origin)));
    for (auto [name, value] :
         {std::pair{shader::UNIFORM_DASH_LENGTH, dashLength}, std::pair{shader::UNIFORM_DASH_GAP, dashGap},
          std::pair{shader::UNIFORM_DASH_OFFSET, dashOffset}})
      if (auto location = glGetUniformLocation(shader.id, name); location != -1) glUniform1f(location, value);

    rect_end(rectVAO, mode, count);
  }

  // A layer draw's crop of its spritesheet. A blurred one (Flash's BlurFilter: a box `blur` canvas pixels wide) grows by
  // the box's reach, in texels at the draw's scale, and averages its crop over it.
  void Canvas::layer_render(Shader& shader, GLuint texture, vec2 textureSize, const mat4& transform,
                            const model::Draw& draw, vec4 tint, vec4 colorOffset) const
  {
    auto blurred = draw;
    auto& frame = blurred.frame;
    TextureBlur blur{.region = vec4(frame.crop, frame.crop + frame.size) / vec4(textureSize, textureSize)};
    if (frame.blur > 0.0f && frame.size.x > 0.0f && frame.size.y > 0.0f)
    {
      auto canvasArea = std::abs(glm::determinant(glm::mat2(draw.parent * model::draw_quad_model_get(draw))));
      auto canvasPerTexel = std::sqrt(canvasArea / (frame.size.x * frame.size.y));
      blur.radius = canvasPerTexel > 0.0f ? std::round(frame.blur * 0.5f / canvasPerTexel) : 0.0f;
      frame.crop -= blur.radius;
      frame.size += blur.radius * 2.0f;
      frame.pivot += blur.radius;
    }
    auto vertices = math::uv_vertices_get(frame.crop / textureSize, (frame.crop + frame.size) / textureSize);
    texture_render(shader, texture, transform * blurred.parent * model::draw_quad_model_get(blurred), tint, colorOffset,
                   vertices.data(), textureSize, draw.time, blur);
  }

  // An animation's layers alone (no markers or helpers); spritesheet images are looked up by draw, custom shaders by id.
  void Canvas::layers_render(Shader& shader, const model::Model& model, const model::Animation& animation,
                             const model::DrawOptions& options, const mat4& transform,
                             const std::function<const Image*(const model::Draw&)>& image_get,
                             const std::function<Shader*(int)>& shader_get) const
  {
    for (const auto& draw : model::animation_draws_get(model, animation, options))
    {
      if (draw.type != model::DrawType::LAYER) continue;
      auto& frame = draw.frame;
      auto image = image_get(draw);
      if (!image || !image->is_valid() || frame.size == vec2()) continue;
      auto customShader = shader_get ? shader_get(frame.shaderId) : nullptr;
      layer_render(customShader ? *customShader : shader, texture::id_get(*image), vec2(image->size), transform, draw,
                   frame.tint, frame.colorOffset);
    }
  }

  void Canvas::rect_render(Shader& shader, const mat4& transform, const mat4& model, vec4 color, float dashLength,
                           float dashGap, float dashOffset) const
  {
    dashed_render(shader, transform, model, color, dashLength, dashGap, dashOffset, GL_LINE_LOOP, RECT_VERTEX_COUNT);
  }

  // A line from start to end: the bottom edge of a rectangle laid along it.
  void Canvas::line_render(Shader& shader, const mat4& transform, vec2 start, vec2 end, vec4 color, float dashLength,
                           float dashGap, float dashOffset) const
  {
    auto direction = end - start;
    auto normal = glm::length(direction) > 0.0f ? glm::normalize(vec2(-direction.y, direction.x)) : vec2(0.0f, 1.0f);
    auto model = mat4(vec4(direction, 0.0f, 0.0f), vec4(normal, 0.0f, 0.0f), vec4(0.0f, 0.0f, 1.0f, 0.0f),
                      vec4(start, 0.0f, 1.0f));
    dashed_render(shader, transform, model, color, dashLength, dashGap, dashOffset, GL_LINES, LINE_VERTEX_COUNT);
  }

  void Canvas::rect_fill_render(Shader& shader, const mat4& transform, const mat4& model, vec4 color) const
  {
    rect_begin(shader, transform, model, color);
    rect_end(rectVAO, GL_TRIANGLE_FAN);
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
