#pragma once

#include "anm2.h"
#include "render.h"
#include "tool.h"

#define SETTINGS_BUFFER 0xFFFF
#define SETTINGS_BUFFER_ITEM 0xFF
#define SETTINGS_SECTION "[Settings]"
#define SETTINGS_SECTION_IMGUI "# Dear ImGui"
#define SETTINGS_INIT_ERROR "Failed to read settings file! ({})"
#define SETTINGS_PATH "settings.ini"
#define SETTINGS_FLOAT_FORMAT "{:.3f}"

struct SettingsEntry
{
    std::string key;
    DataType type;
    s32 offset;
};

struct Settings
{
    ivec2 windowSize = {1080, 720};
    bool playbackIsLoop = true;
    bool playbackIsClampPlayhead = true;
    bool changeIsCrop = false;
    bool changeIsSize = false;
    bool changeIsPosition = false;
    bool changeIsPivot = false;
    bool changeIsScale = false;
    bool changeIsRotation = false;
    bool changeIsDelay = false;
    bool changeIsTint = false;
    bool changeIsColorOffset = false;
    bool changeIsVisibleSet = false;
    bool changeIsInterpolatedSet = false;
    bool changeIsFromSelectedFrame = false;
    vec2 changeCrop{};
    vec2 changeSize{};
    vec2 changePosition{};
    vec2 changePivot{};
    vec2 changeScale{};
    f32 changeRotation{};
    s32 changeDelay{};
    vec4 changeTint{};
    vec3 changeColorOffset{};
    bool changeIsVisible{};
    bool changeIsInterpolated{};
    s32 changeNumberFrames = 1;
    bool previewIsAxes = true;
    bool previewIsGrid = true;
    bool previewIsRootTransform = false;
    bool previewIsTriggers = true;
    bool previewIsPivots = false;
    bool previewIsTargets = true;
    bool previewIsBorder = false;
    f32 previewOverlayTransparency = 255.0f;
    f32 previewZoom = 200.0;
    vec2 previewPan = {0.0, 0.0};
    ivec2 previewGridSize = {32, 3};
    ivec2 previewGridOffset{};
    vec4 previewGridColor = {1.0, 1.0, 1.0, 0.125};
    vec4 previewAxesColor = {1.0, 1.0, 1.0, 0.125};
    vec4 previewBackgroundColor = {0.113, 0.184, 0.286, 1.0};
    ivec2 generateStartPosition = {0, 0};
    ivec2 generateFrameSize = {64, 64};
    ivec2 generatePivot = {32, 32};
    s32 generateRows = 4;
    s32 generateColumns = 4;
    s32 generateFrameCount = 16;
    s32 generateDelay = 1;
    bool editorIsGrid = true;
    bool editorIsGridSnap = true;
    bool editorIsBorder = true;
    f32 editorZoom = 200.0;
    vec2 editorPan = {0.0, 0.0};
    ivec2 editorGridSize = {32, 32};
    ivec2 editorGridOffset = {32, 32};
    vec4 editorGridColor = {1.0, 1.0, 1.0, 0.125};
    vec4 editorBackgroundColor = {0.113, 0.184, 0.286, 1.0};
    s32 mergeType = ANM2_MERGE_APPEND_FRAMES;
    bool mergeIsDeleteAnimationsAfter = false;
    s32 bakeInterval = 1;
    bool bakeIsRoundScale = true;
    bool bakeIsRoundRotation = true;
    s32 tool = TOOL_PAN;
    vec4 toolColor = {1.0, 1.0, 1.0, 1.0}; 
    s32 renderType = RENDER_PNG;
    std::string renderPath = ".";
    std::string renderFormat = "{}.png";
    std::string ffmpegPath = "/usr/bin/ffmpeg";
}; 

const SettingsEntry SETTINGS_ENTRIES[] =
{
    {"window", TYPE_IVEC2, offsetof(Settings, windowSize)},
    {"playbackIsLoop", TYPE_BOOL, offsetof(Settings, playbackIsLoop)},
    {"playbackIsClampPlayhead", TYPE_BOOL, offsetof(Settings, playbackIsClampPlayhead)},
    {"changeIsCrop", TYPE_BOOL, offsetof(Settings, changeIsCrop)},
    {"changeIsSize", TYPE_BOOL, offsetof(Settings, changeIsSize)},
    {"changeIsPosition", TYPE_BOOL, offsetof(Settings, changeIsPosition)},
    {"changeIsPivot", TYPE_BOOL, offsetof(Settings, changeIsPivot)},
    {"changeIsScale", TYPE_BOOL, offsetof(Settings, changeIsScale)},
    {"changeIsRotation", TYPE_BOOL, offsetof(Settings, changeIsRotation)},
    {"changeIsDelay", TYPE_BOOL, offsetof(Settings, changeIsDelay)},
    {"changeIsTint", TYPE_BOOL, offsetof(Settings, changeIsTint)},
    {"changeIsColorOffset", TYPE_BOOL, offsetof(Settings, changeIsColorOffset)},
    {"changeIsVisibleSet", TYPE_BOOL, offsetof(Settings, changeIsVisibleSet)},
    {"changeIsInterpolatedSet", TYPE_BOOL, offsetof(Settings, changeIsInterpolatedSet)},
    {"changeIsFromSelectedFrame", TYPE_BOOL, offsetof(Settings, changeIsFromSelectedFrame)},
    {"changeCrop", TYPE_VEC2, offsetof(Settings, changeCrop)},
    {"changeSize", TYPE_VEC2, offsetof(Settings, changeSize)},
    {"changePosition", TYPE_VEC2, offsetof(Settings, changePosition)},
    {"changePivot", TYPE_VEC2, offsetof(Settings, changePivot)},
    {"changeScale", TYPE_VEC2, offsetof(Settings, changeScale)},
    {"changeRotation", TYPE_FLOAT, offsetof(Settings, changeRotation)},
    {"changeDelay", TYPE_INT, offsetof(Settings, changeDelay)},
    {"changeTint", TYPE_VEC4, offsetof(Settings, changeTint)},
    {"changeColorOffset", TYPE_VEC3, offsetof(Settings, changeColorOffset)},
    {"changeIsVisible", TYPE_BOOL, offsetof(Settings, changeIsVisibleSet)},
    {"changeIsInterpolated", TYPE_BOOL, offsetof(Settings, changeIsInterpolatedSet)},
    {"changeNumberFrames", TYPE_INT, offsetof(Settings, changeNumberFrames)},
    {"previewIsAxes", TYPE_BOOL, offsetof(Settings, previewIsAxes)},
    {"previewIsGrid", TYPE_BOOL, offsetof(Settings, previewIsGrid)},
    {"previewIsRootTransform", TYPE_BOOL, offsetof(Settings, previewIsRootTransform)},
    {"previewIsTriggers", TYPE_BOOL, offsetof(Settings, previewIsTriggers)},
    {"previewIsPivots", TYPE_BOOL, offsetof(Settings, previewIsPivots)},
    {"previewIsTargets", TYPE_BOOL, offsetof(Settings, previewIsTargets)},
    {"previewIsBorder", TYPE_BOOL, offsetof(Settings, previewIsBorder)},
    {"previewOverlayTransparency", TYPE_FLOAT, offsetof(Settings, previewOverlayTransparency)},
    {"previewZoom", TYPE_FLOAT, offsetof(Settings, previewZoom)},
    {"previewPan", TYPE_VEC2, offsetof(Settings, previewPan)},
    {"previewGridSize", TYPE_IVEC2, offsetof(Settings, previewGridSize)},
    {"previewGridOffset", TYPE_IVEC2, offsetof(Settings, previewGridOffset)},
    {"previewGridColor", TYPE_VEC4, offsetof(Settings, previewGridColor)},
    {"previewAxesColor", TYPE_VEC4, offsetof(Settings, previewAxesColor)},
    {"previewBackgroundColor", TYPE_VEC4, offsetof(Settings, previewBackgroundColor)},
    {"generateStartPosition", TYPE_VEC2, offsetof(Settings, generateStartPosition)},
    {"generateFrameSize", TYPE_VEC2, offsetof(Settings, generateFrameSize)},
    {"generatePivot", TYPE_VEC2, offsetof(Settings, generatePivot)},
    {"generateRows", TYPE_INT, offsetof(Settings, generateRows)},
    {"generateColumns", TYPE_INT, offsetof(Settings, generateColumns)},
    {"generateFrameCount", TYPE_INT, offsetof(Settings, generateFrameCount)},
    {"generateDelay", TYPE_INT, offsetof(Settings, generateDelay)},
    {"editorIsGrid", TYPE_BOOL, offsetof(Settings, editorIsGrid)},
    {"editorIsGridSnap", TYPE_BOOL, offsetof(Settings, editorIsGridSnap)},
    {"editorIsBorder", TYPE_BOOL, offsetof(Settings, editorIsBorder)},
    {"editorZoom", TYPE_FLOAT, offsetof(Settings, editorZoom)},
    {"editorPan", TYPE_VEC2, offsetof(Settings, editorPan)},
    {"editorGridSize", TYPE_IVEC2, offsetof(Settings, editorGridSize)},
    {"editorGridOffset", TYPE_IVEC2, offsetof(Settings, editorGridOffset)},
    {"editorGridColor", TYPE_VEC4, offsetof(Settings, editorGridColor)},
    {"editorBackgroundColor", TYPE_VEC4, offsetof(Settings, editorBackgroundColor)},
    {"mergeType", TYPE_INT, offsetof(Settings, mergeType)},
    {"mergeIsDeleteAnimationsAfter", TYPE_BOOL, offsetof(Settings, mergeIsDeleteAnimationsAfter)},
    {"bakeInterval", TYPE_INT, offsetof(Settings, bakeInterval)},
    {"bakeRoundScale", TYPE_BOOL, offsetof(Settings, bakeIsRoundScale)},
    {"bakeRoundRotation", TYPE_BOOL, offsetof(Settings, bakeIsRoundRotation)},
    {"tool", TYPE_INT, offsetof(Settings, tool)},
    {"toolColor", TYPE_VEC4, offsetof(Settings, toolColor)},
    {"renderType", TYPE_INT, offsetof(Settings, renderType)},
    {"renderPath", TYPE_STRING, offsetof(Settings, renderPath)},
    {"renderFormat", TYPE_STRING, offsetof(Settings, renderFormat)},
    {"ffmpegPath", TYPE_STRING, offsetof(Settings, ffmpegPath)}
};
constexpr s32 SETTINGS_COUNT = (s32)std::size(SETTINGS_ENTRIES);

void settings_save(Settings* self);
void settings_init(Settings* self);