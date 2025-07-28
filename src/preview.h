#pragma once

#include "anm2.h"
#include "resources.h"
#include "settings.h"
#include "canvas.h"

const vec2 PREVIEW_SIZE = {2000, 2000};
const vec2 PREVIEW_CENTER = {0, 0};

#define PREVIEW_ZOOM_MIN 1
#define PREVIEW_ZOOM_MAX 1000
#define PREVIEW_ZOOM_STEP 25
#define PREVIEW_GRID_MIN 1
#define PREVIEW_GRID_MAX 1000
#define PREVIEW_GRID_OFFSET_MIN 0
#define PREVIEW_GRID_OFFSET_MAX 100

const vec2 PREVIEW_NULL_RECT_SIZE = {100, 100};
const vec2 PREVIEW_POINT_SIZE = {2, 2};
const vec2 PREVIEW_PIVOT_SIZE = {4, 4};
const vec4 PREVIEW_ROOT_TINT = COLOR_GREEN;
const vec4 PREVIEW_NULL_TINT = COLOR_BLUE;
const vec4 PREVIEW_PIVOT_TINT = COLOR_RED;
const vec2 PREVIEW_TARGET_SIZE = {16, 16};

struct Preview
{
    Anm2* anm2 = nullptr;
    Anm2Reference* reference = nullptr;
    Resources* resources = nullptr;
    Settings* settings = nullptr;
    GLuint axisVAO = ID_NONE;
    GLuint axisVBO = ID_NONE;
    GLuint fbo = ID_NONE;
    GLuint gridVAO = ID_NONE;
    GLuint gridVBO = ID_NONE;
    GLuint rbo = ID_NONE;
    GLuint texture = ID_NONE;
    GLuint rectVAO = ID_NONE;
    GLuint rectVBO = ID_NONE;
    GLuint textureEBO = ID_NONE;
    GLuint textureVAO = ID_NONE;
    GLuint textureVBO = ID_NONE;
    bool isPlaying = false;
    bool isRecording = false;
    vec2 recordSize{};
    f32 time{};
};

void preview_init(Preview* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, Settings* settings);
void preview_draw(Preview* self);
void preview_tick(Preview* self);
void preview_free(Preview* self);
void preview_record_set(Preview* self);