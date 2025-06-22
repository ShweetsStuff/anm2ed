#pragma once

#include "resources.h"
#include "input.h"

static const ivec2 PREVIEW_SIZE = {960, 720};
static const ivec2 PREVIEW_CENTER = {0, 0};

#define PREVIEW_ZOOM_MIN 1
#define PREVIEW_ZOOM_MAX 400
#define PREVIEW_GRID_MIN 1
#define PREVIEW_GRID_MAX 50
#define PREVIEW_ZOOM_STEP 10

static const f32 PREVIEW_AXIS_VERTICES[] = 
{
    -1.0f,  0.0f,
    1.0f,   0.0f,
    0.0f,   -1.0f,
    0.0f,   1.0f
};

struct Preview
{
    GLuint texture;
    GLuint fbo;
    GLuint rbo;
    GLuint gridVAO;
    GLuint gridVBO;
    GLuint axisVAO;
    GLuint axisVBO;
    Input* input;
    Resources* resources;
    bool isGrid = false;
    bool isAxis = true;
    ivec2 viewport = PREVIEW_SIZE;
    f32 zoom = 100;
    vec2 pan = PREVIEW_CENTER;
    ivec2 gridSize = {10, 10};
    ivec2 oldGridSize = {-1, -1};
    s32 gridVertexCount = -1;
    vec4 backgroundColor = {0.113, 0.184, 0.286, 1.0f};
    vec4 gridColor = {1.0, 1.0, 1.0, 0.125f};
    vec4 axisColor = {1.0, 1.0, 1.0, 0.5f};
};

void preview_init(Preview* self, Resources* resources, Input* input);
void preview_draw(Preview* self);
void preview_tick(Preview* self);
void preview_free(Preview* self);
