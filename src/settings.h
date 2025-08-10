#pragma once

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
    bool previewIsAxis = true;
    bool previewIsGrid = true;
    bool previewIsRootTransform = false;
    bool previewIsPivots = false;
    bool previewIsTargets = true;
    bool previewIsBorder = false;
    f32 previewOverlayTransparency = 255.0f;
    f32 previewZoom = 200.0;
    vec2 previewPan = {0.0, 0.0};
    ivec2 previewGridSize = {32, 32};
    ivec2 previewGridOffset{};
    vec4 previewGridColor = {1.0, 1.0, 1.0, 0.125};
    vec4 previewAxisColor = {1.0, 1.0, 1.0, 0.125};
    vec4 previewBackgroundColor = {0.113, 0.184, 0.286, 1.0};
    bool editorIsGrid = true;
    bool editorIsGridSnap = true;
    bool editorIsBorder = true;
    f32 editorZoom = 200.0;
    vec2 editorPan = {0.0, 0.0};
    ivec2 editorGridSize = {32, 32};
    ivec2 editorGridOffset = {32, 32};
    vec4 editorGridColor = {1.0, 1.0, 1.0, 0.125};
    vec4 editorBackgroundColor = {0.113, 0.184, 0.286, 1.0};
    ToolType tool = TOOL_PAN;
    vec4 toolColor = {1.0, 1.0, 1.0, 1.0}; 
    RenderType renderType = RENDER_PNG;
    std::string renderPath = ".";
    std::string renderFormat = "{}.png";
    std::string ffmpegPath = "/usr/bin/ffmpeg";
}; 

const SettingsEntry SETTINGS_ENTRIES[] =
{
    {"window", TYPE_IVEC2, offsetof(Settings, windowSize)},
    {"playbackIsLoop", TYPE_BOOL, offsetof(Settings, playbackIsLoop)},
    {"previewIsAxis", TYPE_BOOL, offsetof(Settings, previewIsAxis)},
    {"previewIsGrid", TYPE_BOOL, offsetof(Settings, previewIsGrid)},
    {"previewIsRootTransform", TYPE_BOOL, offsetof(Settings, previewIsRootTransform)},
    {"previewIsPivots", TYPE_BOOL, offsetof(Settings, previewIsPivots)},
    {"previewIsTargets", TYPE_BOOL, offsetof(Settings, previewIsTargets)},
    {"previewIsBorder", TYPE_BOOL, offsetof(Settings, previewIsBorder)},
    {"previewOverlayTransparency", TYPE_FLOAT, offsetof(Settings, previewOverlayTransparency)},
    {"previewZoom", TYPE_FLOAT, offsetof(Settings, previewZoom)},
    {"previewPan", TYPE_VEC2, offsetof(Settings, previewPan)},
    {"previewGridSize", TYPE_IVEC2, offsetof(Settings, previewGridSize)},
    {"previewGridOffset", TYPE_IVEC2, offsetof(Settings, previewGridOffset)},
    {"previewGridColor", TYPE_VEC4, offsetof(Settings, previewGridColor)},
    {"previewAxisColor", TYPE_VEC4, offsetof(Settings, previewAxisColor)},
    {"previewBackgroundColor", TYPE_VEC4, offsetof(Settings, previewBackgroundColor)},
    {"editorIsGrid", TYPE_BOOL, offsetof(Settings, editorIsGrid)},
    {"editorIsGridSnap", TYPE_BOOL, offsetof(Settings, editorIsGridSnap)},
    {"editorIsBorder", TYPE_BOOL, offsetof(Settings, editorIsBorder)},
    {"editorZoom", TYPE_FLOAT, offsetof(Settings, editorZoom)},
    {"editorPan", TYPE_VEC2, offsetof(Settings, editorPan)},
    {"editorGridSize", TYPE_IVEC2, offsetof(Settings, editorGridSize)},
    {"editorGridOffset", TYPE_IVEC2, offsetof(Settings, editorGridOffset)},
    {"editorGridColor", TYPE_VEC4, offsetof(Settings, editorGridColor)},
    {"editorBackgroundColor", TYPE_VEC4, offsetof(Settings, editorBackgroundColor)},
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