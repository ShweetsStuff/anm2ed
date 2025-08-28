#pragma once

#include "anm2.h"
#include "render.h"
#include "tool.h"

#define SETTINGS_BUFFER 0xFFFF
#define SETTINGS_BUFFER_ITEM 0xFF
#define SETTINGS_SECTION "[Settings]"
#define SETTINGS_SECTION_IMGUI "# Dear ImGui"
#define SETTINGS_INIT_ERROR "Failed to read settings file: {}"
#define SETTINGS_DEFAULT_ERROR "Failed to write default settings file: {}"
#define SETTINGS_SAVE_ERROR "Failed to write settings file: {}"
#define SETTINGS_SAVE_FINALIZE_ERROR "Failed to write settings file: {} ({})"
#define SETTINGS_FLOAT_FORMAT "{:.3f}"
#define SETTINGS_INIT_INFO "Initialized settings from: {}"
#define SETTINGS_DEFAULT_INFO "Using default settings"
#define SETTINGS_DIRECTORY_ERROR "Failed to create settings directory: {} ({})"
#define SETTINGS_SAVE_INFO "Saved settings to: {}"

#define SETTINGS_FOLDER "anm2ed"
#define SETTINGS_PATH "settings.ini"
#define SETTINGS_TEMPORARY_EXTENSION ".tmp"

struct SettingsEntry
{
    std::string key;
    DataType type;
    s32 offset;
};

struct Settings
{
    ivec2 windowSize = {1080, 720};
    bool isVsync = true;
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
    f32 scaleValue = 1.0f;
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
    ivec2 generateSize = {64, 64};
    ivec2 generatePivot = {32, 32};
    s32 generateRows = 4;
    s32 generateColumns = 4;
    s32 generateCount = 16;
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
    std::string ffmpegPath{};
}; 

const SettingsEntry SETTINGS_ENTRIES[] =
{
    {"window", TYPE_IVEC2, offsetof(Settings, windowSize)},
    {"isVsync", TYPE_BOOL, offsetof(Settings, isVsync)},
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
    {"scaleValue", TYPE_FLOAT, offsetof(Settings, scaleValue)},
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
    {"generateStartPosition", TYPE_IVEC2, offsetof(Settings, generateStartPosition)},
    {"generateSize", TYPE_IVEC2, offsetof(Settings, generateSize)},
    {"generatePivot", TYPE_IVEC2, offsetof(Settings, generatePivot)},
    {"generateRows", TYPE_INT, offsetof(Settings, generateRows)},
    {"generateColumns", TYPE_INT, offsetof(Settings, generateColumns)},
    {"generateCount", TYPE_INT, offsetof(Settings, generateCount)},
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

const std::string SETTINGS_DEFAULT = R"(
[Settings]
windowX=1600
windowY=900
isVsync=true
playbackIsLoop=true
playbackIsClampPlayhead=false
changeIsCrop=false
changeIsSize=false
changeIsPosition=false
changeIsPivot=false
changeIsScale=false
changeIsRotation=false
changeIsDelay=false
changeIsTint=false
changeIsColorOffset=false
changeIsVisibleSet=false
changeIsInterpolatedSet=false
changeIsFromSelectedFrame=false
changeCropX=0.000
changeCropY=0.000
changeSizeX=0.000
changeSizeY=0.000
changePositionX=0.000
changePositionY=0.000
changePivotX=0.000
changePivotY=0.000
changeScaleX=0.000
changeScaleY=0.000
changeRotation=0.000
changeDelay=1
changeTintR=0.000
changeTintG=0.000
changeTintB=0.000
changeTintA=0.000
changeColorOffsetR=0.000
changeColorOffsetG=0.000
changeColorOffsetB=0.000
changeIsVisible=false
changeIsInterpolated=false
changeNumberFrames=1
scaleValue=1.000
previewIsAxes=true
previewIsGrid=false
previewIsRootTransform=true
previewIsTriggers=false
previewIsPivots=false
previewIsTargets=true
previewIsBorder=false
previewOverlayTransparency=255.000
previewZoom=400.000
previewPanX=0.000
previewPanY=0.000
previewGridSizeX=32
previewGridSizeY=32
previewGridOffsetX=16
previewGridOffsetY=16
previewGridColorR=1.000
previewGridColorG=1.000
previewGridColorB=1.000
previewGridColorA=0.125
previewAxesColorR=1.000
previewAxesColorG=1.000
previewAxesColorB=1.000
previewAxesColorA=0.125
previewBackgroundColorR=0.114
previewBackgroundColorG=0.184
previewBackgroundColorB=0.286
previewBackgroundColorA=1.000
generateStartPositionX=0
generateStartPositionY=0
generateSizeX=0
generateSizeY=0
generatePivotX=0
generatePivotY=0
generateRows=4
generateColumns=4
generateCount=16
generateDelay=1
editorIsGrid=true
editorIsGridSnap=true
editorIsBorder=true
editorZoom=400.000
editorPanX=0.000
editorPanY=0.000
editorGridSizeX=32
editorGridSizeY=32
editorGridOffsetX=16
editorGridOffsetY=16
editorGridColorR=1.000
editorGridColorG=1.000
editorGridColorB=1.000
editorGridColorA=0.125
editorBackgroundColorR=0.113
editorBackgroundColorG=0.183
editorBackgroundColorB=0.286
editorBackgroundColorA=1.000
mergeType=1
mergeIsDeleteAnimationsAfter=false
bakeInterval=1
bakeRoundScale=true
bakeRoundRotation=true
tool=0
toolColorR=0.000
toolColorG=0.000
toolColorB=0.000
toolColorA=1.000
renderType=0
renderPath=.
renderFormat={}.png
ffmpegPath=

# Dear ImGui
[Window][## Window]
Pos=0,32
Size=1600,868
Collapsed=0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Tools]
Pos=8,40
Size=39,612
Collapsed=0
DockId=0x0000000B,0

[Window][Animations]
Pos=1288,301
Size=304,351
Collapsed=0
DockId=0x0000000A,0

[Window][Events]
Pos=1005,353
Size=281,299
Collapsed=0
DockId=0x00000008,0

[Window][Spritesheets]
Pos=1288,40
Size=304,259
Collapsed=0
DockId=0x00000009,0

[Window][Animation Preview]
Pos=49,40
Size=954,612
Collapsed=0
DockId=0x0000000C,0

[Window][Spritesheet Editor]
Pos=49,40
Size=954,612
Collapsed=0
DockId=0x0000000C,1

[Window][Timeline]
Pos=8,654
Size=1584,238
Collapsed=0
DockId=0x00000004,0

[Window][Frame Properties]
Pos=1005,40
Size=281,311
Collapsed=0
DockId=0x00000007,0

[Docking][Data]
DockSpace         ID=0xFC02A410 Window=0x0E46F4F7 Pos=8,40 Size=1584,852 Split=Y
  DockNode        ID=0x00000003 Parent=0xFC02A410 SizeRef=1902,612 Split=X
    DockNode      ID=0x00000001 Parent=0x00000003 SizeRef=1278,1016 Split=X Selected=0x024430EF
      DockNode    ID=0x00000005 Parent=0x00000001 SizeRef=995,654 Split=X Selected=0x024430EF
        DockNode  ID=0x0000000B Parent=0x00000005 SizeRef=39,654 Selected=0x18A5FDB9
        DockNode  ID=0x0000000C Parent=0x00000005 SizeRef=954,654 CentralNode=1 Selected=0x024430EF
      DockNode    ID=0x00000006 Parent=0x00000001 SizeRef=281,654 Split=Y Selected=0x754E368F
        DockNode  ID=0x00000007 Parent=0x00000006 SizeRef=631,311 Selected=0x754E368F
        DockNode  ID=0x00000008 Parent=0x00000006 SizeRef=631,299 Selected=0x8A65D963
    DockNode      ID=0x00000002 Parent=0x00000003 SizeRef=304,1016 Split=Y Selected=0x4EFD0020
      DockNode    ID=0x00000009 Parent=0x00000002 SizeRef=634,259 Selected=0x4EFD0020
      DockNode    ID=0x0000000A Parent=0x00000002 SizeRef=634,351 Selected=0xC1986EE2
  DockNode        ID=0x00000004 Parent=0xFC02A410 SizeRef=1902,238 Selected=0x4F89F0DC
)";

void settings_save(Settings* self);
void settings_init(Settings* self);
std::string settings_path_get(void);