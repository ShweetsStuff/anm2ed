#pragma once

#include "anm2.h"
#include "render.h"
#include "tool.h"

#define SETTINGS_SECTION "[Settings]"
#define SETTINGS_SECTION_IMGUI "# Dear ImGui"
#define SETTINGS_INIT_WARNING "Unable to read settings file: {}; using default settings"
#define SETTINGS_INIT_ERROR "Unable to read settings file: {}"
#define SETTINGS_SAVE_ERROR "Failed to write settings file: {}"
#define SETTINGS_SAVE_FINALIZE_ERROR "Failed to write settings file: {} ({})"
#define SETTINGS_VALUE_INIT_WARNING "Unknown setting: {}"
#define SETTINGS_FLOAT_FORMAT "{:.3f}"
#define SETTINGS_INIT_INFO "Initialized settings from: {}"
#define SETTINGS_DIRECTORY_ERROR "Failed to create settings directory: {} ({})"
#define SETTINGS_SAVE_INFO "Saved settings to: {}"

#define SETTINGS_FOLDER "anm2ed"
#define SETTINGS_PATH "settings.ini"
#define SETTINGS_TEMPORARY_EXTENSION ".tmp"

#ifdef _WIN32
#define SETTINGS_FFMPEG_PATH_VALUE_DEFAULT "C:\\ffmpeg\\bin\\ffmpeg.exe"
#else
#define SETTINGS_FFMPEG_PATH_VALUE_DEFAULT "/usr/bin/ffmpeg"
#endif

#define SETTINGS_LIST \
    /* name,                  symbol,                       type,              defaultValue */ \
    X(windowSize,             WINDOW_SIZE,                  TYPE_IVEC2_WH,     {1600, 900}) \
    X(isVsync,                IS_VSYNC,                     TYPE_BOOL,         true) \
    \
    X(hotkeyCenterView,       HOTKEY_CENTER_VIEW,           TYPE_STRING,       "Home") \
    X(hotkeyFit,              HOTKEY_FIT,                   TYPE_STRING,       "F") \
    X(hotkeyZoomIn,           HOTKEY_ZOOM_IN,               TYPE_STRING,       "Ctrl++") \
    X(hotkeyZoomOut,          HOTKEY_ZOOM_OUT,              TYPE_STRING,       "Ctrl+-") \
    X(hotkeyPlayPause,        HOTKEY_PLAY_PAUSE,            TYPE_STRING,       "Space") \
    X(hotkeyOnionskin,        HOTKEY_ONIONSKIN,             TYPE_STRING,       "O") \
    X(hotkeyNew,              HOTKEY_NEW,                   TYPE_STRING,       "Ctrl+N") \
    X(hotkeyOpen,             HOTKEY_OPEN,                  TYPE_STRING,       "Ctrl+O") \
    X(hotkeySave,             HOTKEY_SAVE,                  TYPE_STRING,       "Ctrl+S") \
    X(hotkeySaveAs,           HOTKEY_SAVE_AS,               TYPE_STRING,       "Ctrl+Shift+S") \
    X(hotkeyExit,             HOTKEY_EXIT,                  TYPE_STRING,       "Alt+F4") \
    X(hotkeyPan,              HOTKEY_PAN,                   TYPE_STRING,       "P") \
    X(hotkeyMove,             HOTKEY_MOVE,                  TYPE_STRING,       "V") \
    X(hotkeyRotate,           HOTKEY_ROTATE,                TYPE_STRING,       "R") \
    X(hotkeyScale,            HOTKEY_SCALE,                 TYPE_STRING,       "S") \
    X(hotkeyCrop,             HOTKEY_CROP,                  TYPE_STRING,       "C") \
    X(hotkeyDraw,             HOTKEY_DRAW,                  TYPE_STRING,       "B") \
    X(hotkeyErase,            HOTKEY_ERASE,                 TYPE_STRING,       "E") \
    X(hotkeyColorPicker,      HOTKEY_COLOR_PICKER,          TYPE_STRING,       "I") \
    X(hotkeyUndo,             HOTKEY_UNDO,                  TYPE_STRING,       "Ctrl+Z") \
    X(hotkeyRedo,             HOTKEY_REDO,                  TYPE_STRING,       "Ctrl+Shift+Z") \
    X(hotkeyCopy,             HOTKEY_COPY,                  TYPE_STRING,       "Ctrl+C") \
    X(hotkeyCut,              HOTKEY_CUT,                   TYPE_STRING,       "Ctrl+X") \
    X(hotkeyPaste,            HOTKEY_PASTE,                 TYPE_STRING,       "Ctrl+V") \
    \
    X(playbackIsLoop,         PLAYBACK_IS_LOOP,             TYPE_BOOL,         true) \
    X(playbackIsClampPlayhead,PLAYBACK_IS_CLAMP_PLAYHEAD,   TYPE_BOOL,         true) \
    \
    X(changeIsCrop,           CHANGE_IS_CROP,               TYPE_BOOL,         false) \
    X(changeIsSize,           CHANGE_IS_SIZE,               TYPE_BOOL,         false) \
    X(changeIsPosition,       CHANGE_IS_POSITION,           TYPE_BOOL,         false) \
    X(changeIsPivot,          CHANGE_IS_PIVOT,              TYPE_BOOL,         false) \
    X(changeIsScale,          CHANGE_IS_SCALE,              TYPE_BOOL,         false) \
    X(changeIsRotation,       CHANGE_IS_ROTATION,           TYPE_BOOL,         false) \
    X(changeIsDelay,          CHANGE_IS_DELAY,              TYPE_BOOL,         false) \
    X(changeIsTint,           CHANGE_IS_TINT,               TYPE_BOOL,         false) \
    X(changeIsColorOffset,    CHANGE_IS_COLOR_OFFSET,       TYPE_BOOL,         false) \
    X(changeIsVisibleSet,     CHANGE_IS_VISIBLE_SET,        TYPE_BOOL,         false) \
    X(changeIsInterpolatedSet,CHANGE_IS_INTERPOLATED_SET,   TYPE_BOOL,         false) \
    X(changeIsFromSelectedFrame,CHANGE_IS_FROM_SELECTED_FRAME,TYPE_BOOL,       false) \
    X(changeCrop,             CHANGE_CROP,                  TYPE_VEC2,         {}) \
    X(changeSize,             CHANGE_SIZE,                  TYPE_VEC2,         {}) \
    X(changePosition,         CHANGE_POSITION,              TYPE_VEC2,         {}) \
    X(changePivot,            CHANGE_PIVOT,                 TYPE_VEC2,         {}) \
    X(changeScale,            CHANGE_SCALE,                 TYPE_VEC2,         {}) \
    X(changeRotation,         CHANGE_ROTATION,              TYPE_FLOAT,        0.0f) \
    X(changeDelay,            CHANGE_DELAY,                 TYPE_INT,          0) \
    X(changeTint,             CHANGE_TINT,                  TYPE_VEC4,         {}) \
    X(changeColorOffset,      CHANGE_COLOR_OFFSET,          TYPE_VEC3,         {}) \
    X(changeIsVisible,        CHANGE_IS_VISIBLE,            TYPE_BOOL,         false) \
    X(changeIsInterpolated,   CHANGE_IS_INTERPOLATED,       TYPE_BOOL,         false) \
    X(changeNumberFrames,     CHANGE_NUMBER_FRAMES,         TYPE_INT,          1) \
    \
    X(scaleValue,             SCALE_VALUE,                  TYPE_FLOAT,        1.0f) \
    \
    X(previewIsAxes,          PREVIEW_IS_AXES,              TYPE_BOOL,         true) \
    X(previewIsGrid,          PREVIEW_IS_GRID,              TYPE_BOOL,         true) \
    X(previewIsRootTransform, PREVIEW_IS_ROOT_TRANSFORM,    TYPE_BOOL,         false) \
    X(previewIsTriggers,      PREVIEW_IS_TRIGGERS,          TYPE_BOOL,         true) \
    X(previewIsPivots,        PREVIEW_IS_PIVOTS,            TYPE_BOOL,         false) \
    X(previewIsIcons,         PREVIEW_IS_ICONS,           TYPE_BOOL,         true) \
    X(previewIsBorder,        PREVIEW_IS_BORDER,            TYPE_BOOL,         false) \
    X(previewIsAltIcons,      PREVIEW_IS_ALT_ICONS,         TYPE_BOOL,         false) \
    X(previewOverlayTransparency,PREVIEW_OVERLAY_TRANSPARENCY,TYPE_FLOAT,      255.0f) \
    X(previewZoom,            PREVIEW_ZOOM,                 TYPE_FLOAT,        200.0f) \
    X(previewPan,             PREVIEW_PAN,                  TYPE_VEC2,         {}) \
    X(previewGridSize,        PREVIEW_GRID_SIZE,            TYPE_IVEC2,        {32,32}) \
    X(previewGridOffset,      PREVIEW_GRID_OFFSET,          TYPE_IVEC2,        {}) \
    X(previewGridColor,       PREVIEW_GRID_COLOR,           TYPE_VEC4,         {1.0,1.0,1.0,0.125}) \
    X(previewAxesColor,       PREVIEW_AXES_COLOR,           TYPE_VEC4,         {1.0,1.0,1.0,0.125}) \
    X(previewBackgroundColor, PREVIEW_BACKGROUND_COLOR,     TYPE_VEC4,         {0.113,0.184,0.286,1.0}) \
    \
    X(propertiesIsRound,      PROPERTIES_IS_ROUND,          TYPE_BOOL,         true) \
    \
    X(generateStartPosition,  GENERATE_START_POSITION,      TYPE_IVEC2,        {}) \
    X(generateSize,           GENERATE_SIZE,                TYPE_IVEC2,        {64,64}) \
    X(generatePivot,          GENERATE_PIVOT,               TYPE_IVEC2,        {32,32}) \
    X(generateRows,           GENERATE_ROWS,                TYPE_INT,          4) \
    X(generateColumns,        GENERATE_COLUMNS,             TYPE_INT,          4) \
    X(generateCount,          GENERATE_COUNT,               TYPE_INT,          16) \
    X(generateDelay,          GENERATE_DELAY,               TYPE_INT,          1) \
    \
    X(editorIsGrid,           EDITOR_IS_GRID,               TYPE_BOOL,         true) \
    X(editorIsGridSnap,       EDITOR_IS_GRID_SNAP,          TYPE_BOOL,         true) \
    X(editorIsBorder,         EDITOR_IS_BORDER,             TYPE_BOOL,         true) \
    X(editorZoom,             EDITOR_ZOOM,                  TYPE_FLOAT,        200.0f) \
    X(editorPan,              EDITOR_PAN,                   TYPE_VEC2,         {0.0,0.0}) \
    X(editorGridSize,         EDITOR_GRID_SIZE,             TYPE_IVEC2,        {32,32}) \
    X(editorGridOffset,       EDITOR_GRID_OFFSET,           TYPE_IVEC2,        {32,32}) \
    X(editorGridColor,        EDITOR_GRID_COLOR,            TYPE_VEC4,         {1.0,1.0,1.0,0.125}) \
    X(editorBackgroundColor,  EDITOR_BACKGROUND_COLOR,      TYPE_VEC4,         {0.113,0.184,0.286,1.0}) \
    \
    X(mergeType,              MERGE_TYPE,                   TYPE_INT,          ANM2_MERGE_APPEND_FRAMES) \
    X(mergeIsDeleteAnimationsAfter,MERGE_IS_DELETE_ANIMATIONS_AFTER,TYPE_BOOL, false) \
    \
    X(bakeInterval,           BAKE_INTERVAL,                TYPE_INT,          1) \
    X(bakeIsRoundScale,       BAKE_IS_ROUND_SCALE,          TYPE_BOOL,         true) \
    X(bakeIsRoundRotation,    BAKE_IS_ROUND_ROTATION,       TYPE_BOOL,         true) \
    \
    X(timelineAddItemType,    TIMELINE_ADD_ITEM_TYPE,       TYPE_INT,          ANM2_LAYER) \
    X(timelineIsShowUnused,   TIMELINE_IS_SHOW_UNUSED,      TYPE_BOOL,         true) \
    \
    X(onionskinIsEnabled,     ONIONSKIN_IS_ENABLED,         TYPE_BOOL,         false) \
    X(onionskinDrawOrder,     ONIONSKIN_DRAW_ORDER,         TYPE_INT,          ONIONSKIN_BELOW) \
    X(onionskinBeforeCount,   ONIONSKIN_BEFORE_COUNT,       TYPE_INT,          1) \
    X(onionskinAfterCount,    ONIONSKIN_AFTER_COUNT,        TYPE_INT,          1) \
    X(onionskinBeforeColorOffset,ONIONSKIN_BEFORE_COLOR_OFFSET,TYPE_VEC3,      COLOR_RED) \
    X(onionskinAfterColorOffset, ONIONSKIN_AFTER_COLOR_OFFSET,TYPE_VEC3,       COLOR_BLUE) \
    \
    X(tool,                   TOOL,                         TYPE_INT,          TOOL_PAN) \
    X(toolColor,              TOOL_COLOR,                   TYPE_VEC4,         {1.0,1.0,1.0,1.0}) \
    \
    X(renderType,             RENDER_TYPE,                  TYPE_INT,          RENDER_PNG) \
    X(renderPath,             RENDER_PATH,                  TYPE_STRING,       ".") \
    X(renderFormat,           RENDER_FORMAT,                TYPE_STRING,       "{}.png") \
    X(ffmpegPath,             FFMPEG_PATH,                  TYPE_STRING,       SETTINGS_FFMPEG_PATH_VALUE_DEFAULT)

#define X(name, symbol, type, ...) \
const inline DATATYPE_TO_CTYPE(type) SETTINGS_##symbol##_DEFAULT = __VA_ARGS__;
SETTINGS_LIST
#undef X

struct Settings 
{
    #define X(name, symbol, type, ...) \
    DATATYPE_TO_CTYPE(type) name = SETTINGS_##symbol##_DEFAULT;
    SETTINGS_LIST
    #undef X
};
    
struct SettingsEntry
{
    std::string key;
    DataType type;
    s32 offset;
};

const inline SettingsEntry SETTINGS_ENTRIES[] =
{
    #define X(name, symbol, type, ...) \
    { #name, type, offsetof(Settings, name) },
    SETTINGS_LIST
    #undef X
};

constexpr s32 SETTINGS_COUNT = (s32)std::size(SETTINGS_ENTRIES);

#define HOTKEY_LIST \
    X(NONE,     "None")                 \
    X(CENTER_VIEW,     "Center View")   \
    X(FIT,     "Fit")                   \
    X(ZOOM_IN,     "Zoom In")           \
    X(ZOOM_OUT,     "Zoom Out")         \
    X(PLAY_PAUSE,     "Play/Pause")     \
    X(ONIONSKIN,     "Onionskin")       \
    X(NEW,     "New")                   \
    X(OPEN,     "Open")                 \
    X(SAVE,     "Save")                 \
    X(SAVE_AS,     "Save As")           \
    X(EXIT,   "Exit")                   \
    X(PAN,     "Pan")                   \
    X(MOVE,     "Move")                 \
    X(ROTATE,     "Rotate")             \
    X(SCALE,     "Scale")               \
    X(CROP,     "Crop")                 \
    X(DRAW,     "Draw")                 \
    X(ERASE,     "Erase")               \
    X(COLOR_PICKER,     "Color Picker") \
    X(UNDO,     "Undo")                 \
    X(REDO,     "Redo")                 \
    X(COPY,     "Copy")                 \
    X(CUT,     "Cut")                   \
    X(PASTE,     "Paste")               \
       
typedef enum 
{
    #define X(name, str) HOTKEY_##name,
    HOTKEY_LIST
    #undef X
    HOTKEY_COUNT
} HotkeyType;

const inline char* HOTKEY_STRINGS[] = 
{
    #define X(name, str) str,
    HOTKEY_LIST
    #undef X
};

using HotkeyMember = std::string Settings::*;

const inline HotkeyMember SETTINGS_HOTKEY_MEMBERS[HOTKEY_COUNT] =
{
    nullptr,
    &Settings::hotkeyCenterView,
    &Settings::hotkeyFit,
    &Settings::hotkeyZoomIn,
    &Settings::hotkeyZoomOut,
    &Settings::hotkeyPlayPause,
    &Settings::hotkeyOnionskin,
    &Settings::hotkeyNew,
    &Settings::hotkeyOpen,
    &Settings::hotkeySave,
    &Settings::hotkeySaveAs,
    &Settings::hotkeyExit,
    &Settings::hotkeyPan,
    &Settings::hotkeyMove,
    &Settings::hotkeyRotate,
    &Settings::hotkeyScale,
    &Settings::hotkeyCrop,
    &Settings::hotkeyDraw,
    &Settings::hotkeyErase,
    &Settings::hotkeyColorPicker,
    &Settings::hotkeyUndo,
    &Settings::hotkeyRedo,
    &Settings::hotkeyCopy,
    &Settings::hotkeyCut,
    &Settings::hotkeyPaste
};

const std::string SETTINGS_IMGUI_DEFAULT = R"(
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
Size=38,516
Collapsed=0
DockId=0x0000000B,0

[Window][Animations]
Pos=1289,307
Size=303,249
Collapsed=0
DockId=0x0000000A,0

[Window][Events]
Pos=957,264
Size=330,292
Collapsed=0
DockId=0x00000008,2

[Window][Spritesheets]
Pos=1289,40
Size=303,265
Collapsed=0
DockId=0x00000009,0

[Window][Animation Preview]
Pos=48,40
Size=907,516
Collapsed=0
DockId=0x0000000C,0

[Window][Spritesheet Editor]
Pos=48,40
Size=907,516
Collapsed=0
DockId=0x0000000C,1

[Window][Timeline]
Pos=8,558
Size=1584,334
Collapsed=0
DockId=0x00000004,0

[Window][Frame Properties]
Pos=957,40
Size=330,222
Collapsed=0
DockId=0x00000007,0

[Window][Onionskin]
Pos=957,264
Size=330,292
Collapsed=0
DockId=0x00000008,3

[Window][Layers]
Pos=957,264
Size=330,292
Collapsed=0
DockId=0x00000008,0

[Window][Nulls]
Pos=957,264
Size=330,292
Collapsed=0
DockId=0x00000008,1


[Docking][Data]
DockSpace         ID=0xFC02A410 Window=0x0E46F4F7 Pos=8,40 Size=1584,852 Split=Y
  DockNode        ID=0x00000003 Parent=0xFC02A410 SizeRef=1902,680 Split=X
    DockNode      ID=0x00000001 Parent=0x00000003 SizeRef=1017,1016 Split=X Selected=0x024430EF
      DockNode    ID=0x00000005 Parent=0x00000001 SizeRef=1264,654 Split=X Selected=0x024430EF
        DockNode  ID=0x0000000B Parent=0x00000005 SizeRef=38,654 Selected=0x18A5FDB9
        DockNode  ID=0x0000000C Parent=0x00000005 SizeRef=1224,654 CentralNode=1 Selected=0x024430EF
      DockNode    ID=0x00000006 Parent=0x00000001 SizeRef=330,654 Split=Y Selected=0x754E368F
        DockNode  ID=0x00000007 Parent=0x00000006 SizeRef=631,293 Selected=0x754E368F
        DockNode  ID=0x00000008 Parent=0x00000006 SizeRef=631,385 Selected=0xCD8384B1
    DockNode      ID=0x00000002 Parent=0x00000003 SizeRef=303,1016 Split=Y Selected=0x4EFD0020
      DockNode    ID=0x00000009 Parent=0x00000002 SizeRef=634,349 Selected=0x4EFD0020
      DockNode    ID=0x0000000A Parent=0x00000002 SizeRef=634,329 Selected=0xC1986EE2
  DockNode        ID=0x00000004 Parent=0xFC02A410 SizeRef=1902,334 Selected=0x4F89F0DC
  
)";

void settings_save(Settings* self);
void settings_init(Settings* self);
std::string settings_path_get(void);