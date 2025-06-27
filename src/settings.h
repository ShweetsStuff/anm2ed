#pragma once

#include "COMMON.h"

#define SETTINGS_BUFFER 0xFFFF
#define SETTINGS_BUFFER_ITEM 0xFF
#define SETTINGS_SECTION "[Settings]"
#define SETTINGS_SECTION_IMGUI "# Dear ImGui"

enum SettingsValueType
{
    SETTINGS_TYPE_INT,
    SETTINGS_TYPE_FLOAT,
    SETTINGS_TYPE_BOOL,
    SETTINGS_TYPE_STRING
};

struct SettingsEntry
{
    const char* value;
    const char* format;
    SettingsValueType type;
    s32 offset;
};

#define SETTINGS_COUNT (SETTINGS_EDITOR_BACKGROUND_COLOR_A + 1)
enum SettingsItem
{
    SETTINGS_WINDOW_W,
    SETTINGS_WINDOW_H,
    SETTINGS_PREVIEW_IS_AXIS,
    SETTINGS_PREVIEW_IS_GRID,
    SETTINGS_PREVIEW_IS_ROOT_TRANSFORM,
    SETTINGS_PREVIEW_IS_SHOW_PIVOT,
    SETTINGS_PREVIEW_PAN_X,
    SETTINGS_PREVIEW_PAN_Y,
    SETTINGS_PREVIEW_ZOOM,
    SETTINGS_PREVIEW_GRID_SIZE_X,
    SETTINGS_PREVIEW_GRID_SIZE_Y,
    SETTINGS_PREVIEW_GRID_OFFSET_X,
    SETTINGS_PREVIEW_GRID_OFFSET_Y,
    SETTINGS_PREVIEW_GRID_COLOR_R,
    SETTINGS_PREVIEW_GRID_COLOR_G,
    SETTINGS_PREVIEW_GRID_COLOR_B,
    SETTINGS_PREVIEW_GRID_COLOR_A,
    SETTINGS_PREVIEW_AXIS_COLOR_R,
    SETTINGS_PREVIEW_AXIS_COLOR_G,
    SETTINGS_PREVIEW_AXIS_COLOR_B,
    SETTINGS_PREVIEW_AXIS_COLOR_A,
    SETTINGS_PREVIEW_BACKGROUND_COLOR_R,
    SETTINGS_PREVIEW_BACKGROUND_COLOR_G,
    SETTINGS_PREVIEW_BACKGROUND_COLOR_B,
    SETTINGS_PREVIEW_BACKGROUND_COLOR_A,
    SETTINGS_EDITOR_IS_GRID,
    SETTINGS_EDITOR_IS_BORDER,
    SETTINGS_EDITOR_PAN_X,
    SETTINGS_EDITOR_PAN_Y,
    SETTINGS_EDITOR_ZOOM,
    SETTINGS_EDITOR_GRID_SIZE_X,
    SETTINGS_EDITOR_GRID_SIZE_Y,
    SETTINGS_EDITOR_GRID_OFFSET_X,
    SETTINGS_EDITOR_GRID_OFFSET_Y,
    SETTINGS_EDITOR_GRID_COLOR_R,
    SETTINGS_EDITOR_GRID_COLOR_G,
    SETTINGS_EDITOR_GRID_COLOR_B,
    SETTINGS_EDITOR_GRID_COLOR_A,
    SETTINGS_EDITOR_BACKGROUND_COLOR_R,
    SETTINGS_EDITOR_BACKGROUND_COLOR_G,
    SETTINGS_EDITOR_BACKGROUND_COLOR_B,
    SETTINGS_EDITOR_BACKGROUND_COLOR_A
};

struct Settings
{
    s32 windowW = 1920;
    s32 windowH = 1080;
    bool previewIsAxis = true;
    bool previewIsGrid = true;
    bool previewIsRootTransform = false;
    bool previewIsShowPivot = false;
    f32 previewPanX = 0.0f;
    f32 previewPanY = 0.0f;
    f32 previewZoom = 200.0f;
    s32 previewGridSizeX = 32;
    s32 previewGridSizeY = 32;
    s32 previewGridOffsetX = 0;
    s32 previewGridOffsetY = 0;
    f32 previewGridColorR = 1.0f;
    f32 previewGridColorG = 1.0f;
    f32 previewGridColorB = 1.0f;
    f32 previewGridColorA = 0.125f;
    f32 previewAxisColorR = 1.0f;
    f32 previewAxisColorG = 1.0f;
    f32 previewAxisColorB = 1.0f;
    f32 previewAxisColorA = 0.5f;
    f32 previewBackgroundColorR = 0.113f;
    f32 previewBackgroundColorG = 0.184f;
    f32 previewBackgroundColorB = 0.286f;
    f32 previewBackgroundColorA = 1.0f;
    bool editorIsGrid = true;
    bool editorIsBorder = true;
    f32 editorPanX = 0.0f;
    f32 editorPanY = 0.0f;
    f32 editorZoom = 200.0f;
    s32 editorGridSizeX = 32;
    s32 editorGridSizeY = 32;
    s32 editorGridOffsetX = 32;
    s32 editorGridOffsetY = 32;
    f32 editorGridColorR = 1.0f;
    f32 editorGridColorG = 1.0f;
    f32 editorGridColorB = 1.0f;
    f32 editorGridColorA = 0.125f;
    f32 editorBackgroundColorR = 0.113f;
    f32 editorBackgroundColorG = 0.184f;
    f32 editorBackgroundColorB = 0.286f;
    f32 editorBackgroundColorA = 1.0f;
}; 

static const SettingsEntry SETTINGS_ENTRIES[SETTINGS_COUNT] =
{
    {"windowW=", "windowW=%i", SETTINGS_TYPE_INT, offsetof(Settings, windowW)},
    {"windowH=", "windowH=%i", SETTINGS_TYPE_INT, offsetof(Settings, windowH)},
    {"previewIsAxis=", "previewIsAxis=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, previewIsAxis)},
    {"previewIsGrid=", "previewIsGrid=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, previewIsGrid)},
    {"previewIsRootTransform=", "previewIsRootTransform=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, previewIsRootTransform)},
    {"previewIsShowPivot=", "previewIsShowPivot=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, previewIsShowPivot)},
    {"previewPanX=", "previewPanX=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewPanX)},
    {"previewPanY=", "previewPanY=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewPanY)},
    {"previewZoom=", "previewZoom=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewZoom)},
    {"previewGridSizeX=", "previewGridSizeX=%i", SETTINGS_TYPE_INT, offsetof(Settings, previewGridSizeX)},
    {"previewGridSizeY=", "previewGridSizeY=%i", SETTINGS_TYPE_INT, offsetof(Settings, previewGridSizeY)},
    {"previewGridOffsetX=", "previewGridOffsetX=%i", SETTINGS_TYPE_INT, offsetof(Settings, previewGridOffsetX)},
    {"previewGridOffsetY=", "previewGridOffsetY=%i", SETTINGS_TYPE_INT, offsetof(Settings, previewGridOffsetY)},
    {"previewGridColorR=", "previewGridColorR=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewGridColorR)},
    {"previewGridColorG=", "previewGridColorG=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewGridColorG)},
    {"previewGridColorB=", "previewGridColorB=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewGridColorB)},
    {"previewGridColorA=", "previewGridColorA=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewGridColorA)},
    {"previewAxisColorR=", "previewAxisColorR=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewAxisColorR)},
    {"previewAxisColorG=", "previewAxisColorG=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewAxisColorG)},
    {"previewAxisColorB=", "previewAxisColorB=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewAxisColorB)},
    {"previewAxisColorA=", "previewAxisColorA=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewAxisColorA)},
    {"previewBackgroundColorR=", "previewBackgroundColorR=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewBackgroundColorR)},
    {"previewBackgroundColorG=", "previewBackgroundColorG=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewBackgroundColorG)},
    {"previewBackgroundColorB=", "previewBackgroundColorB=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewBackgroundColorB)},
    {"previewBackgroundColorA=", "previewBackgroundColorA=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewBackgroundColorA)},
    {"editorIsGrid=", "editorIsGrid=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, editorIsGrid)},
    {"editorIsBorder=", "editorIsBorder=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, editorIsBorder)},
    {"editorPanX=", "editorPanX=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorPanX)},
    {"editorPanY=", "editorPanY=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorPanY)},
    {"editorZoom=", "editorZoom=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorZoom)},
    {"editorGridSizeX=", "editorGridSizeX=%i", SETTINGS_TYPE_INT, offsetof(Settings, editorGridSizeX)},
    {"editorGridSizeY=", "editorGridSizeY=%i", SETTINGS_TYPE_INT, offsetof(Settings, editorGridSizeY)},
    {"editorGridOffsetX=", "editorGridOffsetX=%i", SETTINGS_TYPE_INT, offsetof(Settings, editorGridOffsetX)},
    {"editorGridOffsetY=", "editorGridOffsetY=%i", SETTINGS_TYPE_INT, offsetof(Settings, editorGridOffsetY)},
    {"editorGridColorR=", "editorGridColorR=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorGridColorR)},
    {"editorGridColorG=", "editorGridColorG=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorGridColorG)},
    {"editorGridColorB=", "editorGridColorB=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorGridColorB)},
    {"editorGridColorA=", "editorGridColorA=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorGridColorA)},
    {"editorBackgroundColorR=", "editorBackgroundColorR=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorBackgroundColorR)},
    {"editorBackgroundColorG=", "editorBackgroundColorG=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorBackgroundColorG)},
    {"editorBackgroundColorB=", "editorBackgroundColorB=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorBackgroundColorB)},
    {"editorBackgroundColorA=", "editorBackgroundColorA=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorBackgroundColorA)}
};

void settings_save(Settings* self);
void settings_load(Settings* self);