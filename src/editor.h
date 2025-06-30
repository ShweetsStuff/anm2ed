#pragma once

#include "anm2.h"
#include "resources.h"
#include "settings.h"

#define EDITOR_ZOOM_MIN 1
#define EDITOR_ZOOM_MAX 1000
#define EDITOR_ZOOM_STEP 25
#define EDITOR_GRID_MIN 1
#define EDITOR_GRID_MAX 1000
#define EDITOR_GRID_OFFSET_MIN 0
#define EDITOR_GRID_OFFSET_MAX 100

static const vec2 EDITOR_SIZE = {5000, 5000};
static const vec2 EDITOR_PIVOT_SIZE = {4, 4};
static const vec4 EDITOR_TEXTURE_TINT = COLOR_OPAQUE;
static const vec4 EDITOR_BORDER_TINT = COLOR_OPAQUE;
static const vec4 EDITOR_FRAME_TINT = COLOR_RED;

struct Editor
{
    Anm2* anm2 = NULL;
    Anm2Reference* reference = NULL;
    Resources* resources = NULL;
    Settings* settings = NULL;
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
};

void editor_init(Editor* self, Anm2* anm2, Anm2Reference* reference, Resources* resources,  Settings* settings);

void editor_draw(Editor* self);
void editor_tick(Editor* self);
void editor_free(Editor* self);