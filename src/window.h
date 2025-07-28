#pragma once

#include "COMMON.h"

#define WINDOW_TITLE "Anm2Ed"
#define WINDOW_TITLE_FORMAT "Anm2Ed ({})"
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define WINDOW_FLAGS SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL

void window_title_from_path_set(SDL_Window* self, const std::string& path);