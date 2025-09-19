#pragma once

#include "COMMON.h"

#define ATLAS_PATH "resources/atlas.png"
#define FONT_PATH "resources/font.ttf"
#define FONT_SIZE 16

const vec2 TEXTURE_ATLAS_SIZE = {96, 160};

enum AtlasType {
  ATLAS_NONE,
  ATLAS_FOLDER,
  ATLAS_ROOT,
  ATLAS_LAYER,
  ATLAS_NULL,
  ATLAS_TRIGGERS,
  ATLAS_VISIBLE,
  ATLAS_INVISIBLE,
  ATLAS_SHOW_RECT,
  ATLAS_HIDE_RECT,
  ATLAS_SHOW_UNUSED,
  ATLAS_HIDE_UNUSED,
  ATLAS_PAN,
  ATLAS_MOVE,
  ATLAS_ROTATE,
  ATLAS_SCALE,
  ATLAS_CROP,
  ATLAS_DRAW,
  ATLAS_ERASE,
  ATLAS_COLOR_PICKER,
  ATLAS_UNDO,
  ATLAS_REDO,
  ATLAS_ANIMATION,
  ATLAS_SPRITESHEET,
  ATLAS_EVENT,
  ATLAS_PLAY,
  ATLAS_PAUSE,
  ATLAS_ADD,
  ATLAS_REMOVE,
  ATLAS_TRIGGER,
  ATLAS_PIVOT,
  ATLAS_SQUARE,
  ATLAS_CIRCLE,
  ATLAS_PICKER,
  ATLAS_FRAME,
  ATLAS_FRAME_ALT,
  ATLAS_TARGET,
  ATLAS_TARGET_ALT,
  ATLAS_COUNT
};

struct AtlasEntry {
  vec2 position;
  vec2 size;
};

const vec2 ATLAS_SIZE_SMALL = {8, 8};
const vec2 ATLAS_SIZE_NORMAL = {16, 16};
const vec2 ATLAS_SIZE_OBLONG = {16, 40};
const vec2 ATLAS_SIZE_BIG = {40, 40};

const inline AtlasEntry ATLAS_ENTRIES[ATLAS_COUNT] = {
    {{0, 0}, ATLAS_SIZE_NORMAL},   {{16, 0}, ATLAS_SIZE_NORMAL},  {{32, 0}, ATLAS_SIZE_NORMAL},  {{48, 0}, ATLAS_SIZE_NORMAL},  {{64, 0}, ATLAS_SIZE_NORMAL},
    {{80, 0}, ATLAS_SIZE_NORMAL},  {{0, 16}, ATLAS_SIZE_NORMAL},  {{16, 16}, ATLAS_SIZE_NORMAL}, {{32, 16}, ATLAS_SIZE_NORMAL}, {{48, 16}, ATLAS_SIZE_NORMAL},
    {{64, 16}, ATLAS_SIZE_NORMAL}, {{80, 16}, ATLAS_SIZE_NORMAL}, {{0, 32}, ATLAS_SIZE_NORMAL},  {{16, 32}, ATLAS_SIZE_NORMAL}, {{32, 32}, ATLAS_SIZE_NORMAL},
    {{48, 32}, ATLAS_SIZE_NORMAL}, {{64, 32}, ATLAS_SIZE_NORMAL}, {{80, 32}, ATLAS_SIZE_NORMAL}, {{0, 48}, ATLAS_SIZE_NORMAL},  {{16, 48}, ATLAS_SIZE_NORMAL},
    {{32, 48}, ATLAS_SIZE_NORMAL}, {{48, 48}, ATLAS_SIZE_NORMAL}, {{64, 48}, ATLAS_SIZE_NORMAL}, {{80, 48}, ATLAS_SIZE_NORMAL}, {{0, 64}, ATLAS_SIZE_NORMAL},
    {{16, 64}, ATLAS_SIZE_NORMAL}, {{32, 64}, ATLAS_SIZE_NORMAL}, {{48, 64}, ATLAS_SIZE_NORMAL}, {{64, 64}, ATLAS_SIZE_NORMAL}, {{80, 64}, ATLAS_SIZE_SMALL},
    {{88, 64}, ATLAS_SIZE_SMALL},  {{80, 72}, ATLAS_SIZE_SMALL},  {{88, 72}, ATLAS_SIZE_SMALL},  {{0, 80}, ATLAS_SIZE_OBLONG},  {{16, 80}, ATLAS_SIZE_OBLONG},
    {{32, 80}, ATLAS_SIZE_OBLONG}, {{48, 80}, ATLAS_SIZE_BIG},    {{48, 120}, ATLAS_SIZE_BIG}};

#define ATLAS_POSITION(type) ATLAS_ENTRIES[type].position
#define ATLAS_SIZE(type) ATLAS_ENTRIES[type].size
#define ATLAS_UV_MIN(type) (ATLAS_POSITION(type) / TEXTURE_ATLAS_SIZE)
#define ATLAS_UV_MAX(type) ((ATLAS_POSITION(type) + ATLAS_SIZE(type)) / TEXTURE_ATLAS_SIZE)
#define ATLAS_UV_ARGS(type) ATLAS_UV_MIN(type), ATLAS_UV_MAX(type)
#define ATLAS_UV_VERTICES(type) UV_VERTICES(ATLAS_UV_MIN(type), ATLAS_UV_MAX(type))

struct ShaderData {
  std::string vertex;
  std::string fragment;
};

enum ShaderType { SHADER_LINE, SHADER_TEXTURE, SHADER_AXIS, SHADER_GRID, SHADER_COUNT };

const std::string SHADER_VERTEX = R"(
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

const std::string SHADER_AXIS_VERTEX = R"(
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

const std::string SHADER_GRID_VERTEX = R"(
#version 330 core
layout ( location = 0 ) in vec2 i_position;
out vec2 clip;

void main() {
    clip = i_position;
    gl_Position = vec4(i_position, 0.0, 1.0);
}
)";

const std::string SHADER_FRAGMENT = R"(
#version 330 core
out vec4 o_fragColor;
uniform vec4 u_color;
void main()
{
    o_fragColor = u_color;
}
)";

const std::string SHADER_TEXTURE_FRAGMENT = R"(
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

const std::string SHADER_GRID_FRAGMENT = R"(
#version 330 core
in vec2 clip;

uniform mat4 u_model;
uniform vec2 u_size; 
uniform vec2 u_offset;
uniform vec4 u_color;

out vec4 o_fragColor;

void main() 
{
    vec4 w = u_model * vec4(clip, 0.0, 1.0);
    w /= w.w;
    vec2 world = w.xy;

    vec2 g = (world - u_offset) / u_size;

    vec2 d = abs(fract(g) - 0.5);
    float distance = min(d.x, d.y);

    float fw = min(fwidth(g.x), fwidth(g.y));
    float alpha = 1.0 - smoothstep(0.0, fw, distance);

    if (alpha <= 0.0) discard;
    o_fragColor = vec4(u_color.rgb, u_color.a * alpha);
}
)";

#define SHADER_UNIFORM_AXIS "u_axis"
#define SHADER_UNIFORM_COLOR "u_color"
#define SHADER_UNIFORM_TRANSFORM "u_transform"
#define SHADER_UNIFORM_TINT "u_tint"
#define SHADER_UNIFORM_COLOR_OFFSET "u_color_offset"
#define SHADER_UNIFORM_OFFSET "u_offset"
#define SHADER_UNIFORM_ORIGIN_NDC "u_origin_ndc"
#define SHADER_UNIFORM_SIZE "u_size"
#define SHADER_UNIFORM_MODEL "u_model"
#define SHADER_UNIFORM_TEXTURE "u_texture"

const ShaderData SHADER_DATA[SHADER_COUNT] = {{SHADER_VERTEX, SHADER_FRAGMENT},
                                              {SHADER_VERTEX, SHADER_TEXTURE_FRAGMENT},
                                              {SHADER_AXIS_VERTEX, SHADER_FRAGMENT},
                                              {SHADER_GRID_VERTEX, SHADER_GRID_FRAGMENT}};
