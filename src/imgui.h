#pragma once

#include "dialog.h"
#include "resources.h"
#include "preview.h"
#include "window.h"
#include "input.h"
#include "settings.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IMGUI_ENABLE_DOCKING
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#define IMGUI_DRAG_SPEED 1.0
#define IMGUI_TASKBAR_HEIGHT 32
#define IMGUI_TIMELINE_OFFSET_Y 24
#define IMGUI_TIMELINE_ANIMATION_LENGTH_WIDTH 200
#define IMGUI_TIMELINE_FPS_WIDTH 100
#define IMGUI_TIMELINE_CREATED_BY_WIDTH 150
#define IMGUI_TIMELINE_FRAME_INDICES_MULTIPLE 5
#define IMGUI_TIMELINE_FRAME_INDICES_STRING_MAX 16
#define IMGUI_PICKER_LINE_SIZE 1.0f
#define IMGUI_FRAME_BORDER 2.0f
#define IMGUI_PICKER_LINE_COLOR IM_COL32(255, 255, 255, 255)

static const vec2 IMGUI_TASKBAR_MARGINS = {8, 4};

static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_SIZE = {1280, 105};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE = {175, 85};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_POSITION = {8, 135};

static const ImVec2 IMGUI_TIMELINE_ELEMENT_NAME_SIZE = {95, 20};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_SIZE = {45, 20};

static const ImVec2 IMGUI_TIMELINE_ELEMENT_LIST_SIZE = {300, 0};
static const ImVec2 IMGUI_TIMELINE_FRAMES_SIZE = {0, 0};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_FRAMES_SIZE = {0, 0};
static const ImVec2 IMGUI_TIMELINE_FRAME_SIZE = {16, 40};
static const ImVec2 IMGUI_TIMELINE_VIEWER_SIZE = {0, 40};
static const ImVec2 IMGUI_TIMELINE_ELEMENTS_TIMELINE_SIZE = {0, 40};
static const ImVec2 IMGUI_TIMELINE_FRAME_INDICES_SIZE = {0, 40};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_SIZE = {300, 40};
static const ImVec2 IMGUI_TIMELINE_SHIFT_ARROWS_SIZE = {64, 40};

static const ImVec2 IMGUI_SPRITESHEET_SIZE = {0, 150};
static const ImVec2 IMGUI_SPRITESHEET_PREVIEW_SIZE = {100, 100};
static const ImVec2 IMGUI_ICON_SIZE = {16, 16};
static const ImVec2 IMGUI_ICON_SMALL_SIZE = {8, 8};
static const ImVec2 IMGUI_ICON_DUMMY_SIZE = {20, 16};
static const ImVec2 IMGUI_ICON_BUTTON_SIZE = {24, 24};
static const ImVec2 IMGUI_IMAGE_TARGET_SIZE = {125, 125};
static const ImVec2 IMGUI_DUMMY_SIZE = {1, 1};

static const ImVec4 IMGUI_FRAME_BORDER_COLOR = {1.0f, 1.0f, 1.0f, 0.5f};
static const ImVec4 IMGUI_FRAME_OVERLAY_COLOR = {0.0f, 0.0f, 0.0f, 0.25f};
static const ImVec4 IMGUI_FRAME_INDICES_OVERLAY_COLOR = {0.113, 0.184, 0.286, 1.0f};
static const ImVec4 IMGUI_FRAME_INDICES_COLOR = {0.113, 0.184, 0.286, 0.5f};


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
    Settings* settings = NULL;
    s32 animationID = -1;
    s32 timelineElementID = -1;
    s32 eventID = -1;
    s32 spritesheetID = -1;
    s32 timelineElementIndex = -1;
    Anm2AnimationType animationType = ANM2_NONE;
    void* frameVector = NULL;
    s32 frameIndex = -1;
};

void imgui_init
(
    Imgui* self,
    Dialog* dialog,
    Resources* resources,
    Input* input,
    Anm2* anm2,
    Preview* preview,
    Settings* settings,
    SDL_Window* window,
    SDL_GLContext* glContext
);

void imgui_tick(Imgui* self);
void imgui_draw(Imgui* self);
void imgui_free(Imgui* self);
