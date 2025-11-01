#pragma once

#include <glad/glad.h>

namespace anm2ed::resource::shader
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

  void main()
  {
      i_uv_out = i_position;
      gl_Position = vec4(i_position, 0.0, 1.0);
  }
  )";

  constexpr auto GRID_FRAGMENT = R"(
  #version 330 core
  in vec2 i_uv_out;

  uniform mat4 u_transform;
  uniform vec2 u_size;
  uniform vec2 u_offset;
  uniform vec4 u_color;

  out vec4 o_fragColor;

  void main()
  {
      vec4 world4 = u_transform * vec4(i_uv_out, 0.0, 1.0);
      vec2 world = world4.xy / world4.w;

      vec2 cell = max(u_size, vec2(1.0));
      vec2 grid = (world - u_offset) / cell;

      vec2 frac = fract(grid);
      vec2 distToLine = min(frac, 1.0 - frac);
      float distance = min(distToLine.x, distToLine.y);

      float fw = min(fwidth(grid.x), fwidth(grid.y));
      float alpha = 1.0 - smoothstep(0.0, fw, distance);

      if (alpha <= 0.0)
          discard;

      o_fragColor = vec4(u_color.rgb, u_color.a * alpha);
  }
  )";

  constexpr auto DASHED_VERTEX = R"(
  #version 330 core
  layout (location = 0) in vec2 i_position;

  out vec2 v_local;

  uniform mat4 u_transform;

  void main()
  {
      v_local = i_position;
      gl_Position = u_transform * vec4(i_position, 0.0, 1.0);
  }
  )";

  constexpr auto DASHED_FRAGMENT = R"(
  #version 330 core
  in vec2 v_local;

  uniform vec4 u_color;
  uniform vec2 u_axis_x;
  uniform vec2 u_axis_y;
  uniform float u_dash_length;
  uniform float u_dash_gap;
  uniform float u_dash_offset;

  out vec4 o_fragColor;

  void main()
  {
      vec2 local = clamp(v_local, 0.0, 1.0);

      float lengthX = max(length(u_axis_x), 1e-4);
      float lengthY = max(length(u_axis_y), 1e-4);

      float dash = max(u_dash_length, 1e-4);
      float gap = max(u_dash_gap, 0.0);
      float period = max(dash + gap, 1e-4);

      vec2 pixel = max(fwidth(v_local), vec2(1e-5));

      float bottomMask = 1.0 - smoothstep(pixel.y, pixel.y * 2.0, local.y);
      float topMask = 1.0 - smoothstep(pixel.y, pixel.y * 2.0, 1.0 - local.y);
      float leftMask = 1.0 - smoothstep(pixel.x, pixel.x * 2.0, local.x);
      float rightMask = 1.0 - smoothstep(pixel.x, pixel.x * 2.0, 1.0 - local.x);

      float perimeterOffset = u_dash_offset;

      float bottomPos = mod(perimeterOffset + local.x * lengthX, period);
      if (bottomPos < 0.0) bottomPos += period;
      float bottomDash = bottomMask * (bottomPos <= dash ? 1.0 : 0.0);

      float rightPos = mod(perimeterOffset + lengthX + local.y * lengthY, period);
      if (rightPos < 0.0) rightPos += period;
      float rightDash = rightMask * (rightPos <= dash ? 1.0 : 0.0);

      float topPos = mod(perimeterOffset + lengthX + lengthY + (1.0 - local.x) * lengthX, period);
      if (topPos < 0.0) topPos += period;
      float topDash = topMask * (topPos <= dash ? 1.0 : 0.0);

      float leftPos = mod(perimeterOffset + 2.0 * lengthX + lengthY + (1.0 - local.y) * lengthY, period);
      if (leftPos < 0.0) leftPos += period;
      float leftDash = leftMask * (leftPos <= dash ? 1.0 : 0.0);

      float alpha = max(max(bottomDash, topDash), max(leftDash, rightDash));

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
  constexpr auto UNIFORM_AXIS_X = "u_axis_x";
  constexpr auto UNIFORM_AXIS_Y = "u_axis_y";
  constexpr auto UNIFORM_DASH_LENGTH = "u_dash_length";
  constexpr auto UNIFORM_DASH_GAP = "u_dash_gap";
  constexpr auto UNIFORM_DASH_OFFSET = "u_dash_offset";

  enum ShaderType
  {
    LINE,
    DASHED,
    TEXTURE,
    AXIS,
    GRID,
    COUNT
  };

  const Info SHADERS[COUNT] = {{VERTEX, FRAGMENT},
                               {DASHED_VERTEX, DASHED_FRAGMENT},
                               {VERTEX, TEXTURE_FRAGMENT},
                               {AXIS_VERTEX, FRAGMENT},
                               {GRID_VERTEX, GRID_FRAGMENT}};
}

namespace anm2ed::resource
{
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
