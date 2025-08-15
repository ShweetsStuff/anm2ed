#pragma once

#include "COMMON.h"

#define WINDOW_TITLE "Anm2Ed"
#define WINDOW_TITLE_FORMAT "Anm2Ed ({})"
#define WINDOW_FLAGS SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL

void window_title_from_path_set(SDL_Window* self, const std::string& path);
bool window_color_from_position_get(SDL_Window* self, vec2 position, vec4* color);
void window_vsync_set(bool isVsync);