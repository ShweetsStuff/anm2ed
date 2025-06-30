#pragma once

#include "dialog.h"
#include "resources.h"
#include "editor.h"
#include "preview.h"
#include "window.h"
#include "input.h"
#include "settings.h"
#include "snapshots.h"
#include "tool.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IMGUI_ENABLE_DOCKING
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#define IMGUI_TIMELINE_ELEMENT_WIDTH 300

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
#define IMGUI_TOOLS_WIDTH_INCREMENT -2

#define IMGUI_POSITION_STRING_MAX 0xFF

static const vec2 IMGUI_TASKBAR_MARGINS = {8, 4};
static const vec2 IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS = {1, 1};

static const ImVec2 IMGUI_RECORD_TOOLTIP_OFFSET = {16, 16};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_SIZE = {1280, 105};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE = {200, 85};
static const ImVec2 IMGUI_ANIMATION_PREVIEW_POSITION = {8, 135};

static const ImVec2 IMGUI_SPRITESHEET_EDITOR_SETTINGS_CHILD_SIZE = {200, 85};
static const ImVec2 IMGUI_SPRITESHEET_EDITOR_SETTINGS_SIZE = {1280, 105};

static const ImVec2 IMGUI_TIMELINE_ELEMENT_LIST_SIZE = {300, 0};
static const ImVec2 IMGUI_TIMELINE_FRAMES_SIZE = {0, 0};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_FRAMES_SIZE = {0, 0};
static const ImVec2 IMGUI_TIMELINE_FRAME_SIZE = {16, 40};
static const ImVec2 IMGUI_TIMELINE_VIEWER_SIZE = {0, 40};
static const ImVec2 IMGUI_TIMELINE_ELEMENTS_TIMELINE_SIZE = {0, 40};
static const ImVec2 IMGUI_TIMELINE_FRAME_INDICES_SIZE = {0, 40};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_SIZE = {300, 40};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_NAME_SIZE = {150, 20};
static const ImVec2 IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_SIZE = {60, 20};

static const ImVec2 IMGUI_SPRITESHEET_SIZE = {0, 150};
static const ImVec2 IMGUI_SPRITESHEET_PREVIEW_SIZE = {100, 100};
static const ImVec2 IMGUI_IMAGE_TARGET_SIZE = {125, 125};
static const ImVec2 IMGUI_ICON_BUTTON_SIZE = {24, 24};
static const ImVec2 IMGUI_DUMMY_SIZE = {1, 1};

static const ImVec4 IMGUI_TIMELINE_HEADER_COLOR = {0.04, 0.04, 0.04, 1.0f};
static const ImVec4 IMGUI_FRAME_BORDER_COLOR = {1.0f, 1.0f, 1.0f, 0.5f};
static const ImVec4 IMGUI_FRAME_OVERLAY_COLOR = {0.0f, 0.0f, 0.0f, 0.25f};
static const ImVec4 IMGUI_FRAME_INDICES_OVERLAY_COLOR = {0.113, 0.184, 0.286, 1.0f};
static const ImVec4 IMGUI_FRAME_INDICES_COLOR = {0.113, 0.184, 0.286, 0.5f};

static const ImVec4 IMGUI_TIMELINE_ROOT_COLOR = {0.010, 0.049, 0.078, 1.0f};
static const ImVec4 IMGUI_TIMELINE_LAYER_COLOR = {0.098, 0.039, 0.020, 1.0f};
static const ImVec4 IMGUI_TIMELINE_NULL_COLOR = {0.020, 0.049, 0.000, 1.0f};
static const ImVec4 IMGUI_TIMELINE_TRIGGERS_COLOR = {0.078, 0.020, 0.029, 1.0f};

static const ImVec4 IMGUI_TIMELINE_ROOT_FRAME_COLOR = {0.020, 0.294, 0.569, 0.5};
static const ImVec4 IMGUI_TIMELINE_LAYER_FRAME_COLOR = {0.529, 0.157, 0.000, 0.5};
static const ImVec4 IMGUI_TIMELINE_NULL_FRAME_COLOR = {0.137, 0.353, 0.000, 0.5};
static const ImVec4 IMGUI_TIMELINE_TRIGGERS_FRAME_COLOR = {0.529, 0.118, 0.196, 0.5};

static const ImVec4 IMGUI_TIMELINE_ROOT_HIGHLIGHT_COLOR = {0.314, 0.588, 0.843, 0.75};
static const ImVec4 IMGUI_TIMELINE_LAYER_HIGHLIGHT_COLOR = {0.882, 0.412, 0.216, 0.75};
static const ImVec4 IMGUI_TIMELINE_NULL_HIGHLIGHT_COLOR = {0.431, 0.647, 0.294, 0.75};
static const ImVec4 IMGUI_TIMELINE_TRIGGERS_HIGHLIGHT_COLOR = {0.804, 0.412, 0.490, 0.75};

static const ImVec4 IMGUI_TIMELINE_ROOT_ACTIVE_COLOR = {0.471, 0.882, 1.000, 0.75};
static const ImVec4 IMGUI_TIMELINE_LAYER_ACTIVE_COLOR = {1.000, 0.618, 0.324, 0.75};
static const ImVec4 IMGUI_TIMELINE_NULL_ACTIVE_COLOR = {0.646, 0.971, 0.441, 0.75};
static const ImVec4 IMGUI_TIMELINE_TRIGGERS_ACTIVE_COLOR = {1.000, 0.618, 0.735, 0.75};


struct Imgui
{  
    Dialog* dialog = NULL;
    Resources* resources = NULL;
    Input* input = NULL;
    Anm2* anm2 = NULL;
    Anm2Reference* reference = NULL;
    f32* time = NULL;
    Editor* editor = NULL;
    Preview* preview = NULL;
    Settings* settings = NULL;
    Tool* tool = NULL;
    Snapshots* snapshots = NULL;
    SDL_Window* window = NULL;
    SDL_GLContext* glContext = NULL;
    bool isSwap = false;
    Anm2Reference swapReference;
};

void 
imgui_init
(
    Imgui* self,
    Dialog* dialog,
    Resources* resources,
    Input* input,
    Anm2* anm2,
    Anm2Reference* reference,
    f32* time,
    Editor* editor,
    Preview* preview,
    Settings* settings,
    Tool* tool,
    Snapshots* snapshots,
    SDL_Window* window,
    SDL_GLContext* glContext
);

void imgui_tick(Imgui* self);
void imgui_draw(Imgui* self);
void imgui_free(Imgui* self);