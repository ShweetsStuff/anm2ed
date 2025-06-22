#pragma once

#include "COMMON.h"

#define SHADER_BUFFER_MAX 2048

bool shader_init(GLuint* self, const char* vertex, const char* fragment);
void shader_free(GLuint* self);