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
static const vec4 EDITOR_TEXTURE_TINT = COLOR_OPAQUE;
static const vec4 EDITOR_BORDER_TINT = COLOR_OPAQUE;
static const vec4 EDITOR_FRAME_TINT = COLOR_RED;

struct Editor
{
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
    s32 gridVertexCount = -1;
    s32 spritesheetID = -1;
    s32 oldSpritesheetID = -1;
    ivec2 oldGridSize = {-1, -1};
    ivec2 oldGridOffset = {-1, -1};
    Anm2Frame frame;
    bool isFrame = false;
};

void editor_init(Editor* self, Resources* resources, Settings* settings);
void editor_draw(Editor* self);
void editor_tick(Editor* self);
void editor_free(Editor* self);