#pragma once

#include "COMMON.h"

#define SHADER_BUFFER_MAX 2048

struct Shader
{
    GLuint handle = 0;
};

bool shader_init(Shader* self, const char* vertex, const char* fragment);
void shader_free(Shader* self);
void shader_use(Shader* self);
