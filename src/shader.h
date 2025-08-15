#pragma once

#include "log.h"

#define SHADER_INFO_LOG_MAX 0xFF
#define SHADER_INIT_ERROR "Failed to initialize shader {}:\n{}"

bool shader_init(GLuint* self, const std::string& vertex, const std::string& fragment);
void shader_free(GLuint* self);