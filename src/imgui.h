#pragma once

#include "dialog.h"
#include "packed.h"
#include "preview.h"
#include "window.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IMGUI_ENABLE_DOCKING
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#define IMGUI_DRAG_SPEED 1.0
#define IMGUI_TASKBAR_HEIGHT 32

static const vec2 IMGUI_TASKBAR_MARGINS = {8, 4};

static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_SIZE = {1280, 64};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_GRID_SIZE = {150, 64};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_ZOOM_SIZE = {150, 64};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_BACKGROUND_SIZE = {150, 64};

static const ImVec2 IMGUI_TIMELINE_ELEMENT_LIST_SIZE = {260, 0};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_SIZE = {230, 64};
static const ImVec2 IMGUI_TIMELINE_SPRITESHEET_ID_SIZE = {50, 20};
static const ImVec2 IMGUI_TIMELINE_SHIFT_ARROWS_SIZE = {24, 48};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_NAME_SIZE = {125, 20};

static const ImVec2 IMGUI_SPRITESHEET_SIZE = {0, 150};
static const ImVec2 IMGUI_ICON_SIZE = {16, 16};
static const ImVec2 IMGUI_ICON_BUTTON_SIZE = {22, 22};
static const ImVec2 IMGUI_DUMMY_SIZE = {1, 1};

#define IMGUI_TIMELINE_SHIFT_ARROWS_WIDTH (IMGUI_TIMELINE_SHIFT_ARROWS_SIZE.x * 1.35)

struct Imgui
{  
    Dialog* dialog = NULL;
    Packed* packed = NULL;
    Anm2* anm2 = NULL;
    Preview* preview = NULL;
    SDL_Window* window = NULL;
};

void imgui_init(SDL_Window* window, SDL_GLContext glContext);
void imgui_tick(Imgui* self);
void imgui_draw(Imgui* self);
void imgui_free(Imgui* self);
