#pragma once

#include "anm2.h"
#include "canvas.h"
#include "resources.h"
#include "settings.h"

#define EDITOR_ZOOM_MIN 1.0f
#define EDITOR_ZOOM_MAX 1000.0f
#define EDITOR_ZOOM_STEP 25.0
#define EDITOR_GRID_MIN 1
#define EDITOR_GRID_MAX 1000
#define EDITOR_GRID_OFFSET_MIN 0
#define EDITOR_GRID_OFFSET_MAX 100

static const vec4 EDITOR_BORDER_COLOR = COLOR_OPAQUE;
static const vec4 EDITOR_FRAME_COLOR = COLOR_RED;
static const vec4 EDITOR_PIVOT_COLOR = COLOR_PINK;

struct Editor
{
    Anm2* anm2 = nullptr;
    Anm2Reference* reference = nullptr;
    Resources* resources = nullptr;
    Settings* settings = nullptr;
    Canvas canvas;
    GLuint fbo;
    GLuint rbo;
    GLuint gridVAO;
    GLuint gridVBO;
    GLuint texture;
    GLuint textureEBO;
    GLuint textureVAO;
    GLuint textureVBO;
    GLuint borderVAO;
    GLuint borderVBO;
    s32 spritesheetID = ID_NONE;
};

void editor_init(Editor* self, Anm2* anm2, Anm2Reference* reference, Resources* resources,  Settings* settings);
void editor_draw(Editor* self);
void editor_free(Editor* self);