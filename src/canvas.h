#pragma once

#include "resources.h"

#define CANVAS_ZOOM_MIN 1.0f
#define CANVAS_ZOOM_MAX 2000.0f
#define CANVAS_ZOOM_STEP 10.0f
#define CANVAS_ZOOM_MOD 10.0f
#define CANVAS_GRID_MIN 1
#define CANVAS_GRID_MAX 1000
#define CANVAS_LINE_LENGTH (FLT_MAX * 0.001f)

static const vec2 CANVAS_GRID_SIZE = {3200, 1600};
static const vec2 CANVAS_PIVOT_SIZE = {4, 4};

const f32 CANVAS_AXIS_VERTICES[] = 
{
    -CANVAS_LINE_LENGTH, 0.0f,
    CANVAS_LINE_LENGTH, 0.0f,
    0.0f, -CANVAS_LINE_LENGTH,
    0.0f, CANVAS_LINE_LENGTH
};

struct Canvas
{
    GLuint fbo{};
    GLuint rbo{};
    GLuint axisVAO{};
    GLuint axisVBO{};
    GLuint rectVAO{};
    GLuint rectVBO{};
    GLuint gridVAO{};
    GLuint gridVBO{};
    GLuint texture{};
    GLuint textureEBO{};
    GLuint textureVAO{};
    GLuint textureVBO{};
    vec2 size{};
};

void canvas_init(Canvas* self);
mat4 canvas_transform_get(Canvas* self, vec2& pan, f32& zoom, OriginType origin);
void canvas_clear(vec4& color);
void canvas_bind(Canvas* self);
void canvas_viewport_set(Canvas* self);
void canvas_unbind(void);
void canvas_texture_set(Canvas* self);
void canvas_grid_draw(Canvas* self, GLuint& shader, mat4& transform, f32& zoom, ivec2& size, ivec2& offset, vec4& color);
void canvas_axes_draw(Canvas* self, GLuint& shader, mat4& transform, vec4& color);
void canvas_rect_draw(Canvas* self, const GLuint& shader, const mat4& transform, const vec4& color);
void canvas_free(Canvas* self);
void canvas_draw(Canvas* self);

mat4 canvas_mvp_get
(
    mat4& transform, 
    vec2 size,
    vec2 position = {0.0f, 0.0f}, 
    vec2 pivot = {0.0f, 0.0f}, 
    f32 rotation = {0.0f},
    vec2 scale = {1.0f, 1.0f},
    vec2 pivotAlt = {0.0f, 0.0f},
    f32 rotationAlt = {0.0f}
);

void canvas_texture_draw
(
    Canvas* self, 
    GLuint& shader, 
    GLuint& texture, 
    mat4& transform, 
    const f32* vertices = GL_UV_VERTICES,
    vec4 tint = COLOR_OPAQUE,
    vec3 colorOffset = COLOR_OFFSET_NONE
);