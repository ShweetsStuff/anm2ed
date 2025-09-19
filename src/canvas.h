#pragma once

#include "resources.h"

#define CANVAS_ZOOM_MIN 1.0f
#define CANVAS_ZOOM_MAX 2000.0f
#define CANVAS_ZOOM_DEFAULT 100.0f
#define CANVAS_ZOOM_STEP 100.0f
#define CANVAS_GRID_MIN 1
#define CANVAS_GRID_MAX 1000
#define CANVAS_GRID_DEFAULT 32

const inline vec2 CANVAS_PIVOT_SIZE = {4, 4};
const inline vec2 CANVAS_SCALE_DEFAULT = {1.0f, 1.0f};

const inline float CANVAS_AXIS_VERTICES[] = {-1.0f, 1.0f};

const inline float CANVAS_GRID_VERTICES[] = {-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};

const inline float CANVAS_RECT_VERTICES[] = {0, 0, 1, 0, 1, 1, 0, 1};

const inline float CANVAS_TEXTURE_VERTICES[] = {0, 0, 0.0f, 0.0f, 1, 0, 1.0f, 0.0f, 1, 1, 1.0f, 1.0f, 0, 1, 0.0f, 1.0f};

struct Canvas {
  GLuint fbo{};
  GLuint rbo{};
  GLuint axisVAO{};
  GLuint axisVBO{};
  GLuint rectVAO{};
  GLuint rectVBO{};
  GLuint gridVAO{};
  GLuint gridVBO{};
  GLuint framebuffer{};
  GLuint textureVAO{};
  GLuint textureVBO{};
  GLuint textureEBO{};
  ivec2 size{};
  ivec2 previousSize{};
  bool isInit = false;
};

#define UV_VERTICES(uvMin, uvMax) {0, 0, uvMin.x, uvMin.y, 1, 0, uvMax.x, uvMin.y, 1, 1, uvMax.x, uvMax.y, 0, 1, uvMin.x, uvMax.y}

#define ATLAS_UV_MIN(type) (ATLAS_POSITION(type) / TEXTURE_ATLAS_SIZE)
#define ATLAS_UV_MAX(type) ((ATLAS_POSITION(type) + ATLAS_SIZE(type)) / TEXTURE_ATLAS_SIZE)
#define ATLAS_UV_ARGS(type) ATLAS_UV_MIN(type), ATLAS_UV_MAX(type)
#define ATLAS_UV_VERTICES(type) UV_VERTICES(ATLAS_UV_MIN(type), ATLAS_UV_MAX(type))

mat4 canvas_transform_get(Canvas* self, vec2 pan, float zoom, OriginType origin);
void canvas_axes_draw(Canvas* self, GLuint& shader, mat4& transform, vec4& color);
void canvas_bind(Canvas* self);
void canvas_clear(vec4& color);
void canvas_draw(Canvas* self);
void canvas_free(Canvas* self);
void canvas_grid_draw(Canvas* self, GLuint& shader, mat4& transform, ivec2& size, ivec2& offset, vec4& color);
void canvas_init(Canvas* self, const ivec2& size);
void canvas_rect_draw(Canvas* self, const GLuint& shader, const mat4& transform, const vec4& color);
void canvas_framebuffer_resize_check(Canvas* self);
void canvas_unbind(void);
void canvas_viewport_set(Canvas* self);

void canvas_texture_draw(Canvas* self, GLuint& shader, GLuint& texture, mat4& transform, const float* vertices = CANVAS_TEXTURE_VERTICES,
                         vec4 tint = COLOR_OPAQUE, vec3 colorOffset = COLOR_OFFSET_NONE);