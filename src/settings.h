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

#define SETTINGS_COUNT (SETTINGS_BACKGROUND_COLOR_A + 1)
enum SettingsItem
{
    SETTINGS_WINDOW_W,
    SETTINGS_WINDOW_H,
    SETTINGS_IS_AXIS,
    SETTINGS_IS_GRID,
    SETTINGS_IS_ROOT_TRANSFORM,
    SETTINGS_IS_SHOW_PIVOT,
    SETTINGS_PAN_X,
    SETTINGS_PAN_Y,
    SETTINGS_ZOOM,
    SETTINGS_GRID_SIZE_X,
    SETTINGS_GRID_SIZE_Y,
    SETTINGS_GRID_OFFSET_X,
    SETTINGS_GRID_OFFSET_Y,
    SETTINGS_GRID_COLOR_R,
    SETTINGS_GRID_COLOR_G,
    SETTINGS_GRID_COLOR_B,
    SETTINGS_GRID_COLOR_A,
    SETTINGS_AXIS_COLOR_R,
    SETTINGS_AXIS_COLOR_G,
    SETTINGS_AXIS_COLOR_B,
    SETTINGS_AXIS_COLOR_A,
    SETTINGS_BACKGROUND_COLOR_R,
    SETTINGS_BACKGROUND_COLOR_G,
    SETTINGS_BACKGROUND_COLOR_B,
    SETTINGS_BACKGROUND_COLOR_A
};

struct Settings
{
    s32 windowW = 1920;
    s32 windowH = 1080;
    bool isAxis = true;
    bool isGrid = true;
    bool isRootTransform = false;
    bool isShowPivot = false;
    f32 panX = 0.0f;
    f32 panY = 0.0f;
    f32 zoom = 200.0f;
    s32 gridSizeX = 10;
    s32 gridSizeY = 10;
    s32 gridOffsetX = 10;
    s32 gridOffsetY = 10;
    f32 gridColorR = 1.0f;
    f32 gridColorG = 1.0f;
    f32 gridColorB = 1.0f;
    f32 gridColorA = 0.125f;
    f32 axisColorR = 1.0f;
    f32 axisColorG = 1.0f;
    f32 axisColorB = 1.0f;
    f32 axisColorA = 0.5f;
    f32 backgroundColorR = 0.113f;
    f32 backgroundColorG = 0.184f;
    f32 backgroundColorB = 0.286f;
    f32 backgroundColorA = 1.0f;
}; 

static const SettingsEntry SETTINGS_ENTRIES[SETTINGS_COUNT] =
{
    {"windowW=", "windowW=%i", SETTINGS_TYPE_INT, offsetof(Settings, windowW)},
    {"windowH=", "windowH=%i", SETTINGS_TYPE_INT, offsetof(Settings, windowH)},
    {"isAxis=", "isAxis=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, isAxis)},
    {"isGrid=", "isGrid=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, isGrid)},
    {"isRootTransform=", "isRootTransform=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, isRootTransform)},
    {"isShowPivot=", "isShowPivot=%i", SETTINGS_TYPE_BOOL, offsetof(Settings, isShowPivot)},
    {"panX=", "panX=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, panX)},
    {"panY=", "panY=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, panY)},
    {"zoom=", "zoom=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, zoom)},
    {"gridSizeX=", "gridSizeX=%i", SETTINGS_TYPE_INT, offsetof(Settings, gridSizeX)},
    {"gridSizeY=", "gridSizeY=%i", SETTINGS_TYPE_INT, offsetof(Settings, gridSizeY)},
    {"gridOffsetX=", "gridOffsetX=%i", SETTINGS_TYPE_INT, offsetof(Settings, gridOffsetX)},
    {"gridOffsetY=", "gridOffsetY=%i", SETTINGS_TYPE_INT, offsetof(Settings, gridOffsetY)},
    {"gridColorR=", "gridColorR=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, gridColorR)},
    {"gridColorG=", "gridColorG=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, gridColorG)},
    {"gridColorB=", "gridColorB=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, gridColorB)},
    {"gridColorA=", "gridColorA=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, gridColorA)},
    {"axisColorR=", "axisColorR=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, axisColorR)},
    {"axisColorG=", "axisColorG=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, axisColorG)},
    {"axisColorB=", "axisColorB=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, axisColorB)},
    {"axisColorA=", "axisColorA=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, axisColorA)},
    {"backgroundColorR=", "backgroundColorR=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, backgroundColorR)},
    {"backgroundColorG=", "backgroundColorG=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, backgroundColorG)},
    {"backgroundColorB=", "backgroundColorB=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, backgroundColorB)},
    {"backgroundColorA=", "backgroundColorA=%f", SETTINGS_TYPE_FLOAT, offsetof(Settings, backgroundColorA)}
};

void settings_save(Settings* self);
void settings_load(Settings* self);