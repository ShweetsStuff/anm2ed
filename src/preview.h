#pragma once

#include "anm2.h"
#include "resources.h"
#include "input.h"
#include "settings.h"

static const vec2 PREVIEW_SIZE = {5000, 5000};
static const vec2 PREVIEW_CENTER = {0, 0};

#define PREVIEW_ZOOM_MIN 1
#define PREVIEW_ZOOM_MAX 1000
#define PREVIEW_ZOOM_STEP 25
#define PREVIEW_GRID_MIN 1
#define PREVIEW_GRID_MAX 1000
#define PREVIEW_GRID_OFFSET_MIN 0
#define PREVIEW_GRID_OFFSET_MAX 100
#define PREVIEW_MOVE_STEP 1
#define PREVIEW_ROTATE_STEP 1
#define PREVIEW_SCALE_STEP 1

static const f32 PREVIEW_AXIS_VERTICES[] = 
{
    -1.0f,  0.0f,
    1.0f,   0.0f,
    0.0f,   -1.0f,
    0.0f,   1.0f
};

static const vec2 PREVIEW_NULL_RECT_SIZE = {100, 100};
static const vec2 PREVIEW_POINT_SIZE = {2, 2};
static const vec2 PREVIEW_PIVOT_SIZE = {4, 4};
static const vec4 PREVIEW_ROOT_TINT = COLOR_GREEN;
static const vec4 PREVIEW_NULL_TINT = COLOR_BLUE;
static const vec4 PREVIEW_PIVOT_TINT = COLOR_RED;
static const vec2 PREVIEW_TARGET_SIZE = {16, 16};

struct Preview
{
    Anm2* anm2 = NULL;
    Anm2Reference* reference = NULL;
    f32* time = NULL;
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
    GLuint rectVAO;
    GLuint rectVBO;
    GLuint textureEBO;
    GLuint textureVAO;
    GLuint textureVBO;
    bool isPlaying = false;
    bool isRecording = false;
    vec2 recordSize = {0.0f, 0.0f};
};

void preview_init(Preview* self, Anm2* anm2, Anm2Reference* reference, f32* time, Resources* resources, Settings* settings);
void preview_draw(Preview* self);
void preview_tick(Preview* self);
void preview_free(Preview* self);
void preview_record_set(Preview* self);