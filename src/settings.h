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
#define SETTINGS_RENDER_FFMPEG_PATH_VALUE_DEFAULT "C:\\ffmpeg\\bin\\ffmpeg.exe"
#else
#define SETTINGS_RENDER_FFMPEG_PATH_VALUE_DEFAULT "/usr/bin/ffmpeg"
#endif

#define SETTINGS_LIST                                                                                                                                          \
  /* Symbol / Name / Type / Default */                                                                                                                         \
  X(WINDOW_SIZE, windowSize, TYPE_IVEC2_WH, {1600, 900})                                                                                                       \
  X(IS_VSYNC, isVsync, TYPE_BOOL, true)                                                                                                                        \
  X(DISPLAY_SCALE, displayScale, TYPE_FLOAT, 1.0f)                                                                                                             \
                                                                                                                                                               \
  X(HOTKEY_CENTER_VIEW, hotkeyCenterView, TYPE_STRING, "Home")                                                                                                 \
  X(HOTKEY_FIT, hotkeyFit, TYPE_STRING, "F")                                                                                                                   \
  X(HOTKEY_ZOOM_IN, hotkeyZoomIn, TYPE_STRING, "Ctrl++")                                                                                                       \
  X(HOTKEY_ZOOM_OUT, hotkeyZoomOut, TYPE_STRING, "Ctrl+-")                                                                                                     \
  X(HOTKEY_PLAY_PAUSE, hotkeyPlayPause, TYPE_STRING, "Space")                                                                                                  \
  X(HOTKEY_ONIONSKIN, hotkeyOnionskin, TYPE_STRING, "O")                                                                                                       \
  X(HOTKEY_NEW, hotkeyNew, TYPE_STRING, "Ctrl+N")                                                                                                              \
  X(HOTKEY_OPEN, hotkeyOpen, TYPE_STRING, "Ctrl+O")                                                                                                            \
  X(HOTKEY_SAVE, hotkeySave, TYPE_STRING, "Ctrl+S")                                                                                                            \
  X(HOTKEY_SAVE_AS, hotkeySaveAs, TYPE_STRING, "Ctrl+Shift+S")                                                                                                 \
  X(HOTKEY_EXIT, hotkeyExit, TYPE_STRING, "Alt+F4")                                                                                                            \
  X(HOTKEY_SHORTEN_FRAME, hotkeyShortenFrame, TYPE_STRING, "F4")                                                                                               \
  X(HOTKEY_EXTEND_FRAME, hotkeyExtendFrame, TYPE_STRING, "F5")                                                                                                 \
  X(HOTKEY_INSERT_FRAME, hotkeyInsertFrame, TYPE_STRING, "F6")                                                                                                 \
  X(HOTKEY_PREVIOUS_FRAME, hotkeyPreviousFrame, TYPE_STRING, "Comma")                                                                                          \
  X(HOTKEY_NEXT_FRAME, hotkeyNextFrame, TYPE_STRING, "Period")                                                                                                 \
  X(HOTKEY_PAN, hotkeyPan, TYPE_STRING, "P")                                                                                                                   \
  X(HOTKEY_MOVE, hotkeyMove, TYPE_STRING, "V")                                                                                                                 \
  X(HOTKEY_ROTATE, hotkeyRotate, TYPE_STRING, "R")                                                                                                             \
  X(HOTKEY_SCALE, hotkeyScale, TYPE_STRING, "S")                                                                                                               \
  X(HOTKEY_CROP, hotkeyCrop, TYPE_STRING, "C")                                                                                                                 \
  X(HOTKEY_DRAW, hotkeyDraw, TYPE_STRING, "B")                                                                                                                 \
  X(HOTKEY_ERASE, hotkeyErase, TYPE_STRING, "E")                                                                                                               \
  X(HOTKEY_COLOR_PICKER, hotkeyColorPicker, TYPE_STRING, "I")                                                                                                  \
  X(HOTKEY_UNDO, hotkeyUndo, TYPE_STRING, "Ctrl+Z")                                                                                                            \
  X(HOTKEY_REDO, hotkeyRedo, TYPE_STRING, "Ctrl+Shift+Z")                                                                                                      \
  X(HOTKEY_COPY, hotkeyCopy, TYPE_STRING, "Ctrl+C")                                                                                                            \
  X(HOTKEY_CUT, hotkeyCut, TYPE_STRING, "Ctrl+X")                                                                                                              \
  X(HOTKEY_PASTE, hotkeyPaste, TYPE_STRING, "Ctrl+V")                                                                                                          \
  X(HOTKEY_SELECT_ALL, hotkeySelectAll, TYPE_STRING, "Ctrl+A")                                                                                                 \
  X(HOTKEY_SELECT_NONE, hotkeySelectNone, TYPE_STRING, "Ctrl+Shift+A")                                                                                         \
                                                                                                                                                               \
  X(PLAYBACK_IS_LOOP, playbackIsLoop, TYPE_BOOL, true)                                                                                                         \
  X(PLAYBACK_IS_CLAMP_PLAYHEAD, playbackIsClampPlayhead, TYPE_BOOL, true)                                                                                      \
                                                                                                                                                               \
  X(CHANGE_IS_CROP, changeIsCrop, TYPE_BOOL, false)                                                                                                            \
  X(CHANGE_IS_SIZE, changeIsSize, TYPE_BOOL, false)                                                                                                            \
  X(CHANGE_IS_POSITION, changeIsPosition, TYPE_BOOL, false)                                                                                                    \
  X(CHANGE_IS_PIVOT, changeIsPivot, TYPE_BOOL, false)                                                                                                          \
  X(CHANGE_IS_SCALE, changeIsScale, TYPE_BOOL, false)                                                                                                          \
  X(CHANGE_IS_ROTATION, changeIsRotation, TYPE_BOOL, false)                                                                                                    \
  X(CHANGE_IS_DELAY, changeIsDelay, TYPE_BOOL, false)                                                                                                          \
  X(CHANGE_IS_TINT, changeIsTint, TYPE_BOOL, false)                                                                                                            \
  X(CHANGE_IS_COLOR_OFFSET, changeIsColorOffset, TYPE_BOOL, false)                                                                                             \
  X(CHANGE_IS_VISIBLE_SET, changeIsVisibleSet, TYPE_BOOL, false)                                                                                               \
  X(CHANGE_IS_INTERPOLATED_SET, changeIsInterpolatedSet, TYPE_BOOL, false)                                                                                     \
  X(CHANGE_IS_FROM_SELECTED_FRAME, changeIsFromSelectedFrame, TYPE_BOOL, false)                                                                                \
  X(CHANGE_CROP, changeCrop, TYPE_VEC2, {})                                                                                                                    \
  X(CHANGE_SIZE, changeSize, TYPE_VEC2, {})                                                                                                                    \
  X(CHANGE_POSITION, changePosition, TYPE_VEC2, {})                                                                                                            \
  X(CHANGE_PIVOT, changePivot, TYPE_VEC2, {})                                                                                                                  \
  X(CHANGE_SCALE, changeScale, TYPE_VEC2, {})                                                                                                                  \
  X(CHANGE_ROTATION, changeRotation, TYPE_FLOAT, 0.0f)                                                                                                         \
  X(CHANGE_DELAY, changeDelay, TYPE_INT, 0)                                                                                                                    \
  X(CHANGE_TINT, changeTint, TYPE_VEC4, {})                                                                                                                    \
  X(CHANGE_COLOR_OFFSET, changeColorOffset, TYPE_VEC3, {})                                                                                                     \
  X(CHANGE_IS_VISIBLE, changeIsVisible, TYPE_BOOL, false)                                                                                                      \
  X(CHANGE_IS_INTERPOLATED, changeIsInterpolated, TYPE_BOOL, false)                                                                                            \
  X(CHANGE_NUMBER_FRAMES, changeNumberFrames, TYPE_INT, 1)                                                                                                     \
                                                                                                                                                               \
  X(SCALE_VALUE, scaleValue, TYPE_FLOAT, 1.0f)                                                                                                                 \
                                                                                                                                                               \
  X(PREVIEW_IS_AXES, previewIsAxes, TYPE_BOOL, true)                                                                                                           \
  X(PREVIEW_IS_GRID, previewIsGrid, TYPE_BOOL, true)                                                                                                           \
  X(PREVIEW_IS_ROOT_TRANSFORM, previewIsRootTransform, TYPE_BOOL, true)                                                                                        \
  X(PREVIEW_IS_TRIGGERS, previewIsTriggers, TYPE_BOOL, true)                                                                                                   \
  X(PREVIEW_IS_PIVOTS, previewIsPivots, TYPE_BOOL, false)                                                                                                      \
  X(PREVIEW_IS_ICONS, previewIsIcons, TYPE_BOOL, true)                                                                                                         \
  X(PREVIEW_IS_BORDER, previewIsBorder, TYPE_BOOL, false)                                                                                                      \
  X(PREVIEW_IS_ALT_ICONS, previewIsAltIcons, TYPE_BOOL, false)                                                                                                 \
  X(PREVIEW_OVERLAY_TRANSPARENCY, previewOverlayTransparency, TYPE_FLOAT, 255.0f)                                                                              \
  X(PREVIEW_ZOOM, previewZoom, TYPE_FLOAT, 200.0f)                                                                                                             \
  X(PREVIEW_PAN, previewPan, TYPE_VEC2, {})                                                                                                                    \
  X(PREVIEW_GRID_SIZE, previewGridSize, TYPE_IVEC2, {32, 32})                                                                                                  \
  X(PREVIEW_GRID_OFFSET, previewGridOffset, TYPE_IVEC2, {})                                                                                                    \
  X(PREVIEW_GRID_COLOR, previewGridColor, TYPE_VEC4, {1.0, 1.0, 1.0, 0.125})                                                                                   \
  X(PREVIEW_AXES_COLOR, previewAxesColor, TYPE_VEC4, {1.0, 1.0, 1.0, 0.125})                                                                                   \
  X(PREVIEW_BACKGROUND_COLOR, previewBackgroundColor, TYPE_VEC4, {0.113, 0.184, 0.286, 1.0})                                                                   \
                                                                                                                                                               \
  X(PROPERTIES_IS_ROUND, propertiesIsRound, TYPE_BOOL, false)                                                                                                  \
                                                                                                                                                               \
  X(GENERATE_START_POSITION, generateStartPosition, TYPE_IVEC2, {})                                                                                            \
  X(GENERATE_SIZE, generateSize, TYPE_IVEC2, {64, 64})                                                                                                         \
  X(GENERATE_PIVOT, generatePivot, TYPE_IVEC2, {32, 32})                                                                                                       \
  X(GENERATE_ROWS, generateRows, TYPE_INT, 4)                                                                                                                  \
  X(GENERATE_COLUMNS, generateColumns, TYPE_INT, 4)                                                                                                            \
  X(GENERATE_COUNT, generateCount, TYPE_INT, 16)                                                                                                               \
  X(GENERATE_DELAY, generateDelay, TYPE_INT, 1)                                                                                                                \
                                                                                                                                                               \
  X(EDITOR_IS_GRID, editorIsGrid, TYPE_BOOL, true)                                                                                                             \
  X(EDITOR_IS_GRID_SNAP, editorIsGridSnap, TYPE_BOOL, true)                                                                                                    \
  X(EDITOR_IS_BORDER, editorIsBorder, TYPE_BOOL, true)                                                                                                         \
  X(EDITOR_ZOOM, editorZoom, TYPE_FLOAT, 200.0f)                                                                                                               \
  X(EDITOR_PAN, editorPan, TYPE_VEC2, {0.0, 0.0})                                                                                                              \
  X(EDITOR_GRID_SIZE, editorGridSize, TYPE_IVEC2, {32, 32})                                                                                                    \
  X(EDITOR_GRID_OFFSET, editorGridOffset, TYPE_IVEC2, {32, 32})                                                                                                \
  X(EDITOR_GRID_COLOR, editorGridColor, TYPE_VEC4, {1.0, 1.0, 1.0, 0.125})                                                                                     \
  X(EDITOR_BACKGROUND_COLOR, editorBackgroundColor, TYPE_VEC4, {0.113, 0.184, 0.286, 1.0})                                                                     \
                                                                                                                                                               \
  X(MERGE_TYPE, mergeType, TYPE_INT, ANM2_MERGE_APPEND)                                                                                                        \
  X(MERGE_IS_DELETE_ANIMATIONS_AFTER, mergeIsDeleteAnimationsAfter, TYPE_BOOL, false)                                                                          \
                                                                                                                                                               \
  X(BAKE_INTERVAL, bakeInterval, TYPE_INT, 1)                                                                                                                  \
  X(BAKE_IS_ROUND_SCALE, bakeIsRoundScale, TYPE_BOOL, true)                                                                                                    \
  X(BAKE_IS_ROUND_ROTATION, bakeIsRoundRotation, TYPE_BOOL, true)                                                                                              \
                                                                                                                                                               \
  X(TIMELINE_ADD_ITEM_TYPE, timelineAddItemType, TYPE_INT, ANM2_LAYER)                                                                                         \
  X(TIMELINE_IS_SHOW_UNUSED, timelineIsShowUnused, TYPE_BOOL, true)                                                                                            \
                                                                                                                                                               \
  X(ONIONSKIN_IS_ENABLED, onionskinIsEnabled, TYPE_BOOL, false)                                                                                                \
  X(ONIONSKIN_DRAW_ORDER, onionskinDrawOrder, TYPE_INT, ONIONSKIN_BELOW)                                                                                       \
  X(ONIONSKIN_BEFORE_COUNT, onionskinBeforeCount, TYPE_INT, 0)                                                                                                 \
  X(ONIONSKIN_AFTER_COUNT, onionskinAfterCount, TYPE_INT, 0)                                                                                                   \
  X(ONIONSKIN_BEFORE_COLOR_OFFSET, onionskinBeforeColorOffset, TYPE_VEC3, COLOR_RED)                                                                           \
  X(ONIONSKIN_AFTER_COLOR_OFFSET, onionskinAfterColorOffset, TYPE_VEC3, COLOR_BLUE)                                                                            \
                                                                                                                                                               \
  X(TOOL, tool, TYPE_INT, TOOL_PAN)                                                                                                                            \
  X(TOOL_COLOR, toolColor, TYPE_VEC4, {1.0, 1.0, 1.0, 1.0})                                                                                                    \
                                                                                                                                                               \
  X(RENDER_TYPE, renderType, TYPE_INT, RENDER_PNG)                                                                                                             \
  X(RENDER_PATH, renderPath, TYPE_STRING, ".")                                                                                                                 \
  X(RENDER_FORMAT, renderFormat, TYPE_STRING, "{}.png")                                                                                                        \
  X(RENDER_IS_USE_ANIMATION_BOUNDS, renderIsUseAnimationBounds, TYPE_BOOL, true)                                                                               \
  X(RENDER_IS_TRANSPARENT, renderIsTransparent, TYPE_BOOL, true)                                                                                               \
  X(RENDER_SCALE, renderScale, TYPE_FLOAT, 1.0f)                                                                                                               \
  X(RENDER_FFMPEG_PATH, renderFFmpegPath, TYPE_STRING, SETTINGS_RENDER_FFMPEG_PATH_VALUE_DEFAULT)

#define X(symbol, name, type, ...) const inline DATATYPE_TO_CTYPE(type) SETTINGS_##symbol##_DEFAULT = __VA_ARGS__;
SETTINGS_LIST
#undef X

struct Settings {
#define X(symbol, name, type, ...) DATATYPE_TO_CTYPE(type) name = SETTINGS_##symbol##_DEFAULT;
  SETTINGS_LIST
#undef X
};

struct SettingsEntry {
  std::string key;
  DataType type;
  int offset;
};

const inline SettingsEntry SETTINGS_ENTRIES[] = {
#define X(symbol, name, type, ...) {#name, type, offsetof(Settings, name)},
    SETTINGS_LIST
#undef X
};

constexpr int SETTINGS_COUNT = (int)std::size(SETTINGS_ENTRIES);

#define HOTKEY_LIST                                                                                                                                            \
  X(NONE, "None")                                                                                                                                              \
  X(CENTER_VIEW, "Center View")                                                                                                                                \
  X(FIT, "Fit")                                                                                                                                                \
  X(ZOOM_IN, "Zoom In")                                                                                                                                        \
  X(ZOOM_OUT, "Zoom Out")                                                                                                                                      \
  X(PLAY_PAUSE, "Play/Pause")                                                                                                                                  \
  X(ONIONSKIN, "Onionskin")                                                                                                                                    \
  X(NEW, "New")                                                                                                                                                \
  X(OPEN, "Open")                                                                                                                                              \
  X(SAVE, "Save")                                                                                                                                              \
  X(SAVE_AS, "Save As")                                                                                                                                        \
  X(EXIT, "Exit")                                                                                                                                              \
  X(SHORTEN_FRAME, "Shorten Frame")                                                                                                                            \
  X(EXTEND_FRAME, "Extend Frame")                                                                                                                              \
  X(INSERT_FRAME, "Insert Frame")                                                                                                                              \
  X(PREVIOUS_FRAME, "Previous Frame")                                                                                                                          \
  X(NEXT_FRAME, "Next Frame")                                                                                                                                  \
  X(PAN, "Pan")                                                                                                                                                \
  X(MOVE, "Move")                                                                                                                                              \
  X(ROTATE, "Rotate")                                                                                                                                          \
  X(SCALE, "Scale")                                                                                                                                            \
  X(CROP, "Crop")                                                                                                                                              \
  X(DRAW, "Draw")                                                                                                                                              \
  X(ERASE, "Erase")                                                                                                                                            \
  X(COLOR_PICKER, "Color Picker")                                                                                                                              \
  X(UNDO, "Undo")                                                                                                                                              \
  X(REDO, "Redo")                                                                                                                                              \
  X(COPY, "Copy")                                                                                                                                              \
  X(CUT, "Cut")                                                                                                                                                \
  X(PASTE, "Paste")                                                                                                                                            \
  X(SELECT_ALL, "Select All")                                                                                                                                  \
  X(SELECT_NONE, "Select None")

typedef enum {
#define X(name, str) HOTKEY_##name,
  HOTKEY_LIST
#undef X
      HOTKEY_COUNT
} HotkeyType;

const inline char* HOTKEY_STRINGS[] = {
#define X(name, str) str,
    HOTKEY_LIST
#undef X
};

using HotkeyMember = std::string Settings::*;

const inline HotkeyMember SETTINGS_HOTKEY_MEMBERS[HOTKEY_COUNT] = {nullptr,
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
                                                                   &Settings::hotkeyShortenFrame,
                                                                   &Settings::hotkeyExtendFrame,
                                                                   &Settings::hotkeyInsertFrame,
                                                                   &Settings::hotkeyPreviousFrame,
                                                                   &Settings::hotkeyNextFrame,
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
                                                                   &Settings::hotkeyPaste,
                                                                   &Settings::hotkeySelectAll,
                                                                   &Settings::hotkeySelectNone};

const inline std::string SETTINGS_IMGUI_DEFAULT = R"(
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