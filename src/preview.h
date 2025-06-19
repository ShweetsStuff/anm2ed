#pragma once

#include "texture.h"

static const ivec2 PREVIEW_SIZE = {1280, 720};

#define PREVIEW_ZOOM_MIN 0
#define PREVIEW_ZOOM_MAX 400
#define PREVIEW_GRID_MIN 0
#define PREVIEW_GRID_MAX 20

struct Preview
{
    GLuint texture;
    GLuint fbo;
    GLuint rbo;
    bool isGrid;
    f32 zoom = 100;
    ivec2 gridSize = {10, 10};
    vec4 color = {0.69, 0.69, 0.69, 1.0f};
    vec4 gridColor = {0.35, 0.35, 0.35, 1.0f};
};

void preview_init(Preview* self);
void preview_draw(Preview* self);
void preview_free(Preview* self);
