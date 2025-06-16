#pragma once

#include "COMMON.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl3.h>
#include <imgui/imgui_impl_opengl3.h>

#define IMGUI_DRAG_SPEED 1.0

void imgui_init(SDL_Window* window, SDL_GLContext glContext);
void imgui_tick(State* state);
void imgui_draw();
void imgui_free();
