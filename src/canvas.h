#pragma once

#include "texture.h"
#include "shader.h"

#define CANVAS_ZOOM_MIN 1
#define CANVAS_ZOOM_MAX 1000
#define CANVAS_ZOOM_STEP 25
#define CANVAS_LINE_LENGTH 100000.0f
#define CANVAS_GRID_RANGE 100
#define CANVAS_GRID_MIN 1
#define CANVAS_GRID_MAX 100

const f32 CANVAS_AXIS_VERTICES[] = 
{
    -CANVAS_LINE_LENGTH,      0.0f,
    CANVAS_LINE_LENGTH,       0.0f,
    0.0f,       -CANVAS_LINE_LENGTH,
    0.0f,        CANVAS_LINE_LENGTH
};

struct Canvas
{
    GLuint fbo;
    GLuint rbo;
    GLuint texture;
    GLuint textureEBO;
    GLuint textureVAO;
    GLuint textureVBO;
    vec2 size;
};

void canvas_init(Canvas* self, vec2 size);