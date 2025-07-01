#pragma once

#include "COMMON.h"

#define SHADER_INFO_LOG_MAX 0xFF

bool shader_init(GLuint* self, const std::string& vertex, const std::string& fragment);
void shader_free(GLuint* self);