#pragma once

#include "COMMON.h"

#define WINDOW_TITLE_MAX 0xFF + PATH_MAX 

void window_title_from_path_set(SDL_Window* self, const char* path);