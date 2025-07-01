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
    std::string key;
    SettingsValueType type;
    s32 offset;
};

#define SETTINGS_COUNT 44
struct Settings
{
    s32 windowW = 1920;
    s32 windowH = 1080;
    bool playbackIsLoop = true;
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
    bool editorIsGridSnap = true;
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

const SettingsEntry SETTINGS_ENTRIES[SETTINGS_COUNT] =
{
    {"windowW=", SETTINGS_TYPE_INT, offsetof(Settings, windowW)},
    {"windowH=", SETTINGS_TYPE_INT, offsetof(Settings, windowH)},
    {"playbackIsLoop=", SETTINGS_TYPE_BOOL, offsetof(Settings, playbackIsLoop)},
    {"previewIsAxis=", SETTINGS_TYPE_BOOL, offsetof(Settings, previewIsAxis)},
    {"previewIsGrid=", SETTINGS_TYPE_BOOL, offsetof(Settings, previewIsGrid)},
    {"previewIsRootTransform=", SETTINGS_TYPE_BOOL, offsetof(Settings, previewIsRootTransform)},
    {"previewIsShowPivot=", SETTINGS_TYPE_BOOL, offsetof(Settings, previewIsShowPivot)},
    {"previewPanX=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewPanX)},
    {"previewPanY=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewPanY)},
    {"previewZoom=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewZoom)},
    {"previewGridSizeX=", SETTINGS_TYPE_INT, offsetof(Settings, previewGridSizeX)},
    {"previewGridSizeY=", SETTINGS_TYPE_INT, offsetof(Settings, previewGridSizeY)},
    {"previewGridOffsetX=", SETTINGS_TYPE_INT, offsetof(Settings, previewGridOffsetX)},
    {"previewGridOffsetY=", SETTINGS_TYPE_INT, offsetof(Settings, previewGridOffsetY)},
    {"previewGridColorR=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewGridColorR)},
    {"previewGridColorG=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewGridColorG)},
    {"previewGridColorB=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewGridColorB)},
    {"previewGridColorA=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewGridColorA)},
    {"previewAxisColorR=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewAxisColorR)},
    {"previewAxisColorG=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewAxisColorG)},
    {"previewAxisColorB=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewAxisColorB)},
    {"previewAxisColorA=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewAxisColorA)},
    {"previewBackgroundColorR=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewBackgroundColorR)},
    {"previewBackgroundColorG=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewBackgroundColorG)},
    {"previewBackgroundColorB=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewBackgroundColorB)},
    {"previewBackgroundColorA=", SETTINGS_TYPE_FLOAT, offsetof(Settings, previewBackgroundColorA)},
    {"editorIsGrid=", SETTINGS_TYPE_BOOL, offsetof(Settings, editorIsGrid)},
    {"editorIsGridSnap=", SETTINGS_TYPE_BOOL, offsetof(Settings, editorIsGridSnap)},
    {"editorIsBorder=", SETTINGS_TYPE_BOOL, offsetof(Settings, editorIsBorder)},
    {"editorPanX=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorPanX)},
    {"editorPanY=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorPanY)},
    {"editorZoom=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorZoom)},
    {"editorGridSizeX=", SETTINGS_TYPE_INT, offsetof(Settings, editorGridSizeX)},
    {"editorGridSizeY=", SETTINGS_TYPE_INT, offsetof(Settings, editorGridSizeY)},
    {"editorGridOffsetX=", SETTINGS_TYPE_INT, offsetof(Settings, editorGridOffsetX)},
    {"editorGridOffsetY=", SETTINGS_TYPE_INT, offsetof(Settings, editorGridOffsetY)},
    {"editorGridColorR=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorGridColorR)},
    {"editorGridColorG=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorGridColorG)},
    {"editorGridColorB=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorGridColorB)},
    {"editorGridColorA=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorGridColorA)},
    {"editorBackgroundColorR=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorBackgroundColorR)},
    {"editorBackgroundColorG=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorBackgroundColorG)},
    {"editorBackgroundColorB=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorBackgroundColorB)},
    {"editorBackgroundColorA=", SETTINGS_TYPE_FLOAT, offsetof(Settings, editorBackgroundColorA)}
};

void settings_save(Settings* self);
void settings_load(Settings* self);