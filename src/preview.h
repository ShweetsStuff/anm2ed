#pragma once

#include "anm2.h"
#include "resources.h"
#include "settings.h"
#include "canvas.h"

const vec2 PREVIEW_SIZE = {2000, 2000};
const vec2 PREVIEW_CANVAS_SIZE = {2000, 2000};
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
const vec2 PREVIEW_TARGET_SIZE = {16, 16};
const vec4 PREVIEW_BORDER_COLOR = COLOR_RED;
const vec4 PREVIEW_ROOT_COLOR = COLOR_GREEN;
const vec4 PREVIEW_NULL_COLOR = COLOR_BLUE;
const vec4 PREVIEW_NULL_SELECTED_COLOR = COLOR_RED;
const vec4 PREVIEW_PIVOT_COLOR = COLOR_RED;

struct Preview
{
    Anm2* anm2 = nullptr;
    Anm2Reference* reference = nullptr;
    Resources* resources = nullptr;
    Settings* settings = nullptr;
    s32 animationOverlayID = ID_NONE;
    Canvas canvas;
    bool isPlaying = false;
    bool isRender = false;
    bool isRenderFinished = false;
    bool isRenderCancelled = false;
    std::vector<Texture> renderFrames;
    f32 time{};
};

void preview_init(Preview* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, Settings* settings);
void preview_draw(Preview* self);
void preview_tick(Preview* self);
void preview_free(Preview* self);
void preview_render_start(Preview* self);
void preview_render_end(Preview* self);