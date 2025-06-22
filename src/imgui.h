#pragma once

#include "dialog.h"
#include "resources.h"
#include "preview.h"
#include "window.h"
#include "input.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IMGUI_ENABLE_DOCKING
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#define IMGUI_DRAG_SPEED 1.0
#define IMGUI_TASKBAR_HEIGHT 32
#define IMGUI_TIMELINE_OFFSET_Y 32
#define IMGUI_TIMELINE_ANIMATION_LENGTH_WIDTH 200

static const vec2 IMGUI_TASKBAR_MARGINS = {8, 4};

static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_SIZE = {1280, 80};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE = {150, 64};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_POSITION = {8, 110};

static const ImVec2 IMGUI_TIMELINE_FRAME_SIZE = {20, 40};
static const ImVec2 IMGUI_TIMELINE_SIZE = {0, 0};
static const ImVec2 IMGUI_TIMELINE_ELEMENTS_BAR_SIZE = {300, 32};
static const ImVec2 IMGUI_TIMELINE_ELEMENTS_TIMELINE_SIZE = {0, 40};
static const ImVec2 IMGUI_TIMELINE_FRAME_INDICES_SIZE = {0, 32};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_LIST_SIZE = {300, 0};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_SIZE = {300, 40};
static const ImVec2 IMGUI_TIMELINE_SPRITESHEET_ID_SIZE = {40, 20};
static const ImVec2 IMGUI_TIMELINE_SHIFT_ARROWS_SIZE = {64, 40};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_NAME_SIZE = {85, 20};

static const ImVec2 IMGUI_SPRITESHEET_SIZE = {0, 150};
static const ImVec2 IMGUI_SPRITESHEET_PREVIEW_SIZE = {100, 100};
static const ImVec2 IMGUI_ICON_SIZE = {16, 16};
static const ImVec2 IMGUI_ICON_DUMMY_SIZE = {20, 16};
static const ImVec2 IMGUI_ICON_BUTTON_SIZE = {24, 24};
static const ImVec2 IMGUI_IMAGE_TARGET_SIZE = {125, 125};
static const ImVec2 IMGUI_DUMMY_SIZE = {1, 1};

#define IMGUI_TIMELINE_SHIFT_ARROWS_WIDTH (IMGUI_TIMELINE_SHIFT_ARROWS_SIZE.x * 1.35)

struct Imgui
{  
    Dialog* dialog = NULL;
    Resources* resources = NULL;
    Input* input = NULL;
    Anm2* anm2 = NULL;
    Preview* preview = NULL;
    SDL_Window* window = NULL;
    SDL_GLContext* glContext = NULL;
};

void imgui_init
(
    Imgui* self,
    Dialog* dialog,
    Resources* resources,
    Input* input,
    Anm2* anm2,
    Preview* preview,
    SDL_Window* window,
    SDL_GLContext* glContext
);

void imgui_tick(Imgui* self);
void imgui_draw(Imgui* self);
void imgui_free(Imgui* self);
