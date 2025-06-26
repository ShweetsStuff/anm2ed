#pragma once

#include "anm2.h"
#include "resources.h"
#include "input.h"
#include "settings.h"

static const ivec2 PREVIEW_SIZE = {2000, 2000};
static const ivec2 PREVIEW_CENTER = {0, 0};

#define PREVIEW_ZOOM_MIN 1.0f
#define PREVIEW_ZOOM_MAX 800.0f
#define PREVIEW_GRID_MIN 1
#define PREVIEW_GRID_MAX 50
#define PREVIEW_GRID_OFFSET_MIN 0
#define PREVIEW_GRID_OFFSET_MAX 100
#define PREVIEW_ZOOM_STEP 50
#define PREVIEW

static const f32 PREVIEW_AXIS_VERTICES[] = 
{
    -1.0f,  0.0f,
    1.0f,   0.0f,
    0.0f,   -1.0f,
    0.0f,   1.0f
};

static const GLuint PREVIEW_TEXTURE_INDICES[] = {0, 1, 2, 2, 3, 0};

static const vec2 PREVIEW_PIVOT_SIZE = {6, 6};
static const vec2 PREVIEW_TARGET_SIZE = {16, 16};
static const vec4 PREVIEW_ROOT_TINT = {0.0f, 1.0f, 0.0f, 1.0f};
static const vec3 PREVIEW_ROOT_COLOR_OFFSET = {0.0f, 0.0f, 0.0f};
static const vec4 PREVIEW_NULL_TINT = {0.0f, 0.0f, 1.0f, 1.0f};
static const vec3 PREVIEW_NULL_COLOR_OFFSET = {0.0f, 0.0f, 0.0f};
static const vec4 PREVIEW_PIVOT_TINT = {1.0f, 1.0f, 1.0f, 1.0f};
static const vec3 PREVIEW_PIVOT_COLOR_OFFSET = {0.0f, 0.0f, 0.0f};

struct Preview
{
    Anm2* anm2 = NULL;
    Input* input = NULL;
    Resources* resources = NULL;
    Settings* settings = NULL;
    GLuint axisVAO;
    GLuint axisVBO;
    GLuint fbo;
    GLuint gridVAO;
    GLuint gridVBO;
    GLuint rbo;
    GLuint texture;
    GLuint textureEBO;
    GLuint textureVAO;
    GLuint textureVBO;
    bool isPlaying = false;
    f32 time = 0;
    ivec2 oldGridOffset = {-1, -1};
    ivec2 oldGridSize = {-1, -1};
    ivec2 viewport = PREVIEW_SIZE;
    s32 animationID = -1;
    s32 gridVertexCount = -1;

};

void preview_init(Preview* self, Anm2* anm2, Resources* resources, Input* input, Settings* settings);
void preview_draw(Preview* self);
void preview_tick(Preview* self);
void preview_free(Preview* self);
