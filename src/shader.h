#pragma once

#include <glad/glad.h>

namespace anm2ed::shader
{
  struct Info
  {
    const char* vertex;
    const char* fragment;
  };

  constexpr auto VERTEX = R"(
  #version 330 core
  layout (location = 0) in vec2 i_position;
  layout (location = 1) in vec2 i_uv;
  out vec2 i_uv_out;
  uniform mat4 u_transform;
  void main()
  {
      i_uv_out = i_uv;
      gl_Position = u_transform * vec4(i_position, 0.0, 1.0);
  }
  )";

  constexpr auto AXIS_VERTEX = R"(
  #version 330 core
  layout (location = 0) in vec2 i_position; // full screen line segment: -1..1
  uniform vec2 u_origin_ndc; // world origin in NDC
  uniform int u_axis;       // 0 = X axis, 1 = Y axis

  void main()
  {
      vec2 pos = (u_axis == 0)
          ? vec2(i_position.x, u_origin_ndc.y) // horizontal line across screen
          : vec2(u_origin_ndc.x, i_position.x); // vertical line across screen;

      gl_Position = vec4(pos, 0.0, 1.0);
  }
  )";

  constexpr auto FRAGMENT = R"(
  #version 330 core
  out vec4 o_fragColor;
  uniform vec4 u_color;
  void main()
  {
      o_fragColor = u_color;
  }
  )";

  constexpr auto TEXTURE_FRAGMENT = R"(
  #version 330 core
  in vec2 i_uv_out;
  uniform sampler2D u_texture;
  uniform vec4 u_tint;
  uniform vec3 u_color_offset;
  out vec4 o_fragColor;
  void main()
  {
      vec4 texColor = texture(u_texture, i_uv_out);
      texColor *= u_tint;
      texColor.rgb += u_color_offset;
      o_fragColor = texColor;
  }
  )";

  constexpr auto GRID_VERTEX = R"(
#version 330 core
layout (location = 0) in vec2 i_position;
layout (location = 1) in vec2 i_uv;

out vec2 i_uv_out;

void main() {
  i_uv_out = i_position;
  gl_Position = vec4(i_position, 0.0, 1.0);
}
  )";

  constexpr auto GRID_FRAGMENT = R"(
  #version 330 core
  in vec2 i_uv_out;

  uniform vec2 u_view_size;
  uniform vec2 u_pan;
  uniform float u_zoom;
  uniform vec2 u_size;
  uniform vec2 u_offset;
  uniform vec4 u_color;

  out vec4 o_fragColor;

  void main()
  {
      vec2 viewSize = max(u_view_size, vec2(1.0));
      float zoom = max(u_zoom, 1e-6);
      vec2 pan = u_pan;

      vec2 world = (i_uv_out - (2.0 * pan / viewSize)) * (viewSize / (2.0 * zoom));
      world += vec2(0.5); // Half pixel nudge

      vec2 cell = max(u_size, vec2(1.0));
      vec2 grid = (world - u_offset) / cell;

      vec2 d = abs(fract(grid) - 0.5);
      float distance = min(d.x, d.y);

      float fw = min(fwidth(grid.x), fwidth(grid.y));
      float alpha = 1.0 - smoothstep(0.0, fw, distance);

      if (alpha <= 0.0)
          discard;

      o_fragColor = vec4(u_color.rgb, u_color.a * alpha);
  }
  )";

  constexpr auto UNIFORM_AXIS = "u_axis";
  constexpr auto UNIFORM_COLOR = "u_color";
  constexpr auto UNIFORM_TRANSFORM = "u_transform";
  constexpr auto UNIFORM_TINT = "u_tint";
  constexpr auto UNIFORM_COLOR_OFFSET = "u_color_offset";
  constexpr auto UNIFORM_OFFSET = "u_offset";
  constexpr auto UNIFORM_ORIGIN_NDC = "u_origin_ndc";
  constexpr auto UNIFORM_SIZE = "u_size";
  constexpr auto UNIFORM_MODEL = "u_model";
  constexpr auto UNIFORM_RECT_SIZE = "u_rect_size";
  constexpr auto UNIFORM_TEXTURE = "u_texture";
  constexpr auto UNIFORM_VIEW_SIZE = "u_view_size";
  constexpr auto UNIFORM_PAN = "u_pan";
  constexpr auto UNIFORM_ZOOM = "u_zoom";

  enum Type
  {
    LINE,
    TEXTURE,
    AXIS,
    GRID,
    COUNT
  };

  const Info SHADERS[COUNT] = {
      {VERTEX, FRAGMENT}, {VERTEX, TEXTURE_FRAGMENT}, {AXIS_VERTEX, FRAGMENT}, {GRID_VERTEX, GRID_FRAGMENT}};

  class Shader
  {
  public:
    GLuint id{};

    Shader();
    Shader(const char*, const char*);
    Shader& operator=(Shader&&) noexcept;
    ~Shader();
    bool is_valid() const;
  };
}
