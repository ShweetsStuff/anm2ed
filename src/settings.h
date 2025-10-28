#pragma once

#include <string>

#include <glm/glm.hpp>

#include "anm2.h"
#include "types.h"

namespace anm2ed::settings
{
#ifdef _WIN32
  constexpr auto FFMPEG_PATH_DEFAULT = "C:\\ffmpeg\\bin\\ffmpeg.exe";
#else
  constexpr auto FFMPEG_PATH_DEFAULT = "/usr/bin/ffmpeg";
#endif

#define SETTINGS_TYPES                                                                                                 \
  X(INT, int)                                                                                                          \
  X(BOOL, bool)                                                                                                        \
  X(FLOAT, float)                                                                                                      \
  X(STRING, std::string)                                                                                               \
  X(IVEC2, glm::ivec2)                                                                                                 \
  X(IVEC2_WH, glm::ivec2)                                                                                              \
  X(VEC2, glm::vec2)                                                                                                   \
  X(VEC2_WH, glm::vec2)                                                                                                \
  X(VEC3, glm::vec3)                                                                                                   \
  X(VEC4, glm::vec4)

  enum Type
  {
#define X(name, type) name,
    SETTINGS_TYPES
#undef X
  };

#define X(name, type) using TYPE_##name = type;
  SETTINGS_TYPES
#undef X

#define SETTINGS_MEMBERS                                                                                               \
  /* Symbol / Name / String / Type / Default */                                                                        \
  X(WINDOW_SIZE, windowSize, "Window Size", IVEC2_WH, {1600, 900})                                                     \
  X(IS_VSYNC, isVsync, "Vsync", BOOL, true)                                                                            \
  X(DISPLAY_SCALE, displayScale, "Display Scale", FLOAT, 1.0f)                                                         \
                                                                                                                       \
  X(FILE_IS_AUTOSAVE, fileIsAutosave, "Autosave", BOOL, true)                                                          \
  X(FILE_AUTOSAVE_TIME, fileAutosaveTime, "Autosave Time", INT, 1)                                                     \
                                                                                                                       \
  X(VIEW_ZOOM_STEP, viewZoomStep, "Zoom Step", FLOAT, 50.0f)                                                           \
                                                                                                                       \
  X(PLAYBACK_IS_LOOP, playbackIsLoop, "Loop", BOOL, true)                                                              \
  X(PLAYBACK_IS_CLAMP_PLAYHEAD, playbackIsClampPlayhead, "Clamp Playhead", BOOL, true)                                 \
                                                                                                                       \
  X(CHANGE_IS_CROP, changeIsCrop, "##Is Crop", BOOL, false)                                                            \
  X(CHANGE_IS_SIZE, changeIsSize, "##Is Size", BOOL, false)                                                            \
  X(CHANGE_IS_POSITION, changeIsPosition, "##Is Position", BOOL, false)                                                \
  X(CHANGE_IS_PIVOT, changeIsPivot, "##Is Pivot", BOOL, false)                                                         \
  X(CHANGE_IS_SCALE, changeIsScale, "##Is Scale", BOOL, false)                                                         \
  X(CHANGE_IS_ROTATION, changeIsRotation, "##Is Rotation", BOOL, false)                                                \
  X(CHANGE_IS_DELAY, changeIsDelay, "##Is Delay", BOOL, false)                                                         \
  X(CHANGE_IS_TINT, changeIsTint, "##Is Tint", BOOL, false)                                                            \
  X(CHANGE_IS_COLOR_OFFSET, changeIsColorOffset, "##Is Color Offset", BOOL, false)                                     \
  X(CHANGE_IS_VISIBLE_SET, changeIsVisibleSet, "##Is Visible", BOOL, false)                                            \
  X(CHANGE_IS_INTERPOLATED_SET, changeIsInterpolatedSet, "##Is Interpolated", BOOL, false)                             \
  X(CHANGE_CROP, changeCrop, "Crop", VEC2, {})                                                                         \
  X(CHANGE_SIZE, changeSize, "Size", VEC2, {})                                                                         \
  X(CHANGE_POSITION, changePosition, "Position", VEC2, {})                                                             \
  X(CHANGE_PIVOT, changePivot, "Pivot", VEC2, {})                                                                      \
  X(CHANGE_SCALE, changeScale, "Scale", VEC2, {})                                                                      \
  X(CHANGE_ROTATION, changeRotation, "Rotation", FLOAT, 0.0f)                                                          \
  X(CHANGE_DELAY, changeDelay, "Delay", INT, 0)                                                                        \
  X(CHANGE_TINT, changeTint, "Tint", VEC4, {})                                                                         \
  X(CHANGE_COLOR_OFFSET, changeColorOffset, "Color Offset", VEC3, {})                                                  \
  X(CHANGE_IS_VISIBLE, changeIsVisible, "Visible", BOOL, false)                                                        \
  X(CHANGE_IS_INTERPOLATED, changeIsInterpolated, "Interpolated", BOOL, false)                                         \
  X(CHANGE_NUMBER_FRAMES, changeNumberFrames, "Frame Count", INT, 1)                                                   \
  X(CHANGE_IS_FROM_SELECTED_FRAME, changeIsFromSelectedFrame, "From Selected Frame", BOOL, false)                      \
                                                                                                                       \
  X(SCALE_VALUE, scaleValue, "Scale", FLOAT, 1.0f)                                                                     \
                                                                                                                       \
  X(PREVIEW_IS_AXES, previewIsAxes, "Axes", BOOL, true)                                                                \
  X(PREVIEW_IS_GRID, previewIsGrid, "Grid", BOOL, true)                                                                \
  X(PREVIEW_IS_ROOT_TRANSFORM, previewIsRootTransform, "Root Transform", BOOL, true)                                   \
  X(PREVIEW_IS_PIVOTS, previewIsPivots, "Pivots", BOOL, false)                                                         \
  X(PREVIEW_IS_BORDER, previewIsBorder, "Border", BOOL, false)                                                         \
  X(PREVIEW_IS_ALT_ICONS, previewIsAltIcons, "Alt Icons", BOOL, false)                                                 \
  X(PREVIEW_OVERLAY_TRANSPARENCY, previewOverlayTransparency, "Alpha", FLOAT, 255)                                     \
  X(PREVIEW_ZOOM, previewZoom, "Zoom", FLOAT, 200.0f)                                                                  \
  X(PREVIEW_PAN, previewPan, "Pan", VEC2, {})                                                                          \
  X(PREVIEW_GRID_SIZE, previewGridSize, "Size", IVEC2, {32, 32})                                                       \
  X(PREVIEW_GRID_OFFSET, previewGridOffset, "Offset", IVEC2, {})                                                       \
  X(PREVIEW_GRID_COLOR, previewGridColor, "Color", VEC4, {1.0f, 1.0f, 1.0f, 0.125f})                                   \
  X(PREVIEW_AXES_COLOR, previewAxesColor, "Color", VEC4, {1.0f, 1.0f, 1.0f, 0.125f})                                   \
  X(PREVIEW_BACKGROUND_COLOR, previewBackgroundColor, "Background Color", VEC4, {0.113f, 0.184f, 0.286f, 1.0f})        \
                                                                                                                       \
  X(PROPERTIES_IS_ROUND, propertiesIsRound, "Round", BOOL, false)                                                      \
                                                                                                                       \
  X(GENERATE_START_POSITION, generateStartPosition, "Start Position", IVEC2, {})                                       \
  X(GENERATE_SIZE, generateSize, "Size", IVEC2, {64, 64})                                                              \
  X(GENERATE_PIVOT, generatePivot, "Pivot", IVEC2, {32, 32})                                                           \
  X(GENERATE_ROWS, generateRows, "Rows", INT, 4)                                                                       \
  X(GENERATE_COLUMNS, generateColumns, "Columns", INT, 4)                                                              \
  X(GENERATE_COUNT, generateCount, "Count", INT, 16)                                                                   \
  X(GENERATE_DELAY, generateDelay, "Delay", INT, 1)                                                                    \
  X(GENERATE_ZOOM, generateZoom, "Zoom", FLOAT, 100.0f)                                                                \
                                                                                                                       \
  X(EDITOR_IS_GRID, editorIsGrid, "Grid", BOOL, true)                                                                  \
  X(EDITOR_IS_GRID_SNAP, editorIsGridSnap, "Snap", BOOL, true)                                                         \
  X(EDITOR_IS_BORDER, editorIsBorder, "Border", BOOL, true)                                                            \
  X(EDITOR_ZOOM, editorZoom, "Zoom", FLOAT, 200.0f)                                                                    \
  X(EDITOR_PAN, editorPan, "Pan", VEC2, {0.0, 0.0})                                                                    \
  X(EDITOR_GRID_SIZE, editorGridSize, "Size", IVEC2, {32, 32})                                                         \
  X(EDITOR_GRID_OFFSET, editorGridOffset, "Offset", IVEC2, {32, 32})                                                   \
  X(EDITOR_GRID_COLOR, editorGridColor, "Color", VEC4, {1.0, 1.0, 1.0, 0.125})                                         \
  X(EDITOR_BACKGROUND_COLOR, editorBackgroundColor, "Background Color", VEC4, {0.113, 0.184, 0.286, 1.0})              \
                                                                                                                       \
  X(MERGE_TYPE, mergeType, "Type", INT, 0)                                                                             \
  X(MERGE_IS_DELETE_ANIMATIONS_AFTER, mergeIsDeleteAnimationsAfter, "Delete Animations After", BOOL, false)            \
                                                                                                                       \
  X(BAKE_INTERVAL, bakeInterval, "Interval", INT, 1)                                                                   \
  X(BAKE_IS_ROUND_SCALE, bakeIsRoundScale, "Round Scale", BOOL, true)                                                  \
  X(BAKE_IS_ROUND_ROTATION, bakeIsRoundRotation, "Round Rotation", BOOL, true)                                         \
                                                                                                                       \
  X(TIMELINE_ADD_ITEM_TYPE, timelineAddItemType, "Add Item Type", INT, anm2::LAYER)                                    \
  X(TIMELINE_ADD_ITEM_LOCALITY, timelineAddItemLocale, "Add Item Locale", INT, types::locale::GLOBAL)                  \
  X(TIMELINE_ADD_ITEM_SOURCE, timelineAddItemSource, "Add Item Source", INT, types::source::NEW)                       \
  X(TIMELINE_IS_SHOW_UNUSED, timelineIsShowUnused, "##Show Unused", BOOL, true)                                        \
  X(TIMELINE_IS_ONLY_SHOW_LAYERS, timelineIsOnlyShowLayers, "##Only Show Layers", BOOL, true)                          \
                                                                                                                       \
  X(ONIONSKIN_IS_ENABLED, onionskinIsEnabled, "Enabled", BOOL, false)                                                  \
  X(ONIONSKIN_DRAW_ORDER, onionskinDrawOrder, "Draw Order", INT, 0)                                                    \
  X(ONIONSKIN_BEFORE_COUNT, onionskinBeforeCount, "Frames", INT, 0)                                                    \
  X(ONIONSKIN_AFTER_COUNT, onionskinAfterCount, "Frames", INT, 0)                                                      \
  X(ONIONSKIN_BEFORE_COLOR, onionskinBeforeColor, "Color", VEC3, types::color::RED)                                    \
  X(ONIONSKIN_AFTER_COLOR, onionskinAfterColor, "Color", VEC3, types::color::BLUE)                                     \
                                                                                                                       \
  X(TOOL, tool, "##Tool", INT, 0)                                                                                      \
  X(TOOL_COLOR, toolColor, "##Color", VEC4, {1.0, 1.0, 1.0, 1.0})                                                      \
                                                                                                                       \
  X(RENDER_TYPE, renderType, "Output", INT, 0)                                                                         \
  X(RENDER_PATH, renderPath, "Path", STRING, ".")                                                                      \
  X(RENDER_FORMAT, renderFormat, "Format", STRING, "{}.png")                                                           \
  X(RENDER_IS_USE_ANIMATION_BOUNDS, renderIsUseAnimationBounds, "Use Animation Bounds", BOOL, true)                    \
  X(RENDER_IS_TRANSPARENT, renderIsTransparent, "Transparent", BOOL, true)                                             \
  X(RENDER_SCALE, renderScale, "Scale", FLOAT, 1.0f)                                                                   \
  X(RENDER_FFMPEG_PATH, renderFFmpegPath, "FFmpeg Path", STRING, FFMPEG_PATH_DEFAULT)

#define SETTINGS_SHORTCUTS                                                                                             \
  /* Symbol / Name / String / Type / Default */                                                                        \
  X(SHORTCUT_CENTER_VIEW, shortcutCenterView, "Center View", STRING, "Home")                                           \
  X(SHORTCUT_FIT, shortcutFit, "Fit", STRING, "F")                                                                     \
  X(SHORTCUT_ZOOM_IN, shortcutZoomIn, "Zoom In", STRING, "Ctrl++")                                                     \
  X(SHORTCUT_ZOOM_OUT, shortcutZoomOut, "Zoom Out", STRING, "Ctrl+-")                                                  \
  X(SHORTCUT_PLAY_PAUSE, shortcutPlayPause, "Play/Pause", STRING, "Space")                                             \
  X(SHORTCUT_ONIONSKIN, shortcutOnionskin, "Onionskin", STRING, "O")                                                   \
  X(SHORTCUT_NEW, shortcutNew, "New", STRING, "Ctrl+N")                                                                \
  X(SHORTCUT_OPEN, shortcutOpen, "Open", STRING, "Ctrl+O")                                                             \
  X(SHORTCUT_CLOSE, shortcutClose, "Close", STRING, "Ctrl+W")                                                          \
  X(SHORTCUT_SAVE, shortcutSave, "Save", STRING, "Ctrl+S")                                                             \
  X(SHORTCUT_SAVE_AS, shortcutSaveAs, "Save As", STRING, "Ctrl+Shift+S")                                               \
  X(SHORTCUT_EXIT, shortcutExit, "Exit", STRING, "Alt+F4")                                                             \
  X(SHORTCUT_SHORTEN_FRAME, shortcutShortenFrame, "Shorten Frame", STRING, "F4")                                       \
  X(SHORTCUT_EXTEND_FRAME, shortcutExtendFrame, "Extend Frame", STRING, "F5")                                          \
  X(SHORTCUT_INSERT_FRAME, shortcutInsertFrame, "Insert Frame", STRING, "F6")                                          \
  X(SHORTCUT_PREVIOUS_FRAME, shortcutPreviousFrame, "Previous Frame", STRING, "Comma")                                 \
  X(SHORTCUT_NEXT_FRAME, shortcutNextFrame, "Next Frame", STRING, "Period")                                            \
  X(SHORTCUT_PAN, shortcutPan, "Pan", STRING, "P")                                                                     \
  X(SHORTCUT_MOVE, shortcutMove, "Move", STRING, "V")                                                                  \
  X(SHORTCUT_ROTATE, shortcutRotate, "Rotate", STRING, "R")                                                            \
  X(SHORTCUT_SCALE, shortcutScale, "Scale", STRING, "S")                                                               \
  X(SHORTCUT_CROP, shortcutCrop, "Crop", STRING, "C")                                                                  \
  X(SHORTCUT_DRAW, shortcutDraw, "Draw", STRING, "B")                                                                  \
  X(SHORTCUT_ERASE, shortcutErase, "Erase", STRING, "E")                                                               \
  X(SHORTCUT_COLOR_PICKER, shortcutColorPicker, "Color Picker", STRING, "I")                                           \
  X(SHORTCUT_UNDO, shortcutUndo, "Undo", STRING, "Ctrl+Z")                                                             \
  X(SHORTCUT_REDO, shortcutRedo, "Redo", STRING, "Ctrl+Shift+Z")                                                       \
  X(SHORTCUT_COLOR, shortcutColor, "Color", STRING, "X")                                                               \
  X(SHORTCUT_COPY, shortcutCopy, "Copy", STRING, "Ctrl+C")                                                             \
  X(SHORTCUT_CUT, shortcutCut, "Cut", STRING, "Ctrl+X")                                                                \
  X(SHORTCUT_ADD, shortcutAdd, "Add", STRING, "Insert")                                                                \
  X(SHORTCUT_REMOVE, shortcutRemove, "Remove", STRING, "Delete")                                                       \
  X(SHORTCUT_DUPLICATE, shortcutDuplicate, "Duplicate", STRING, "Ctrl+J")                                              \
  X(SHORTCUT_DEFAULT, shortcutDefault, "Default", STRING, "Home")                                                      \
  X(SHORTCUT_MERGE, shortcutMerge, "Merge", STRING, "Ctrl+E")                                                          \
  X(SHORTCUT_PASTE, shortcutPaste, "Paste", STRING, "Ctrl+V")                                                          \
  X(SHORTCUT_SELECT_ALL, shortcutSelectAll, "Select All", STRING, "Ctrl+A")                                            \
  X(SHORTCUT_SELECT_NONE, shortcutSelectNone, "Select None", STRING, "Escape")

#define SETTINGS_WINDOWS                                                                                               \
  /* Symbol / Name / String / Type / Default */                                                                        \
  X(WINDOW_ANIMATIONS, windowIsAnimations, "Animations", BOOL, true)                                                   \
  X(WINDOW_ANIMATION_PREVIEW, windowIsAnimationPreview, "Animation Preview", BOOL, true)                               \
  X(WINDOW_EVENTS, windowIsEvents, "Events", BOOL, true)                                                               \
  X(WINDOW_FRAME_PROPERTIES, windowIsFrameProperties, "Frame Properties", BOOL, true)                                  \
  X(WINDOW_LAYERS, windowIsLayers, "Layers", BOOL, true)                                                               \
  X(WINDOW_NULLS, windowIsNulls, "Nulls", BOOL, true)                                                                  \
  X(WINDOW_ONIONSKIN, windowIsOnionskin, "Onionskin", BOOL, true)                                                      \
  X(WINDOW_PREVIEW, windowIsSpritesheets, "Spritesheets", BOOL, true)                                                  \
  X(WINDOW_SPRITESHEET_EDITOR, windowIsSpritesheetEditor, "Spritesheet Editor", BOOL, true)                            \
  X(WINDOW_TIMELINE, windowIsTimeline, "Timeline", BOOL, true)                                                         \
  X(WINDOW_TOOLS, windowIsTools, "Tools", BOOL, true)

  class Settings
  {
  public:
#define X(symbol, name, string, type, ...) TYPE_##type name = __VA_ARGS__;
    SETTINGS_MEMBERS SETTINGS_SHORTCUTS SETTINGS_WINDOWS
#undef X

    Settings();

    Settings(const std::string&);
    void save(const std::string&, const std::string&);
  };

  enum ShortcutType
  {
#define X(symbol, name, string, type, ...) symbol,
    SETTINGS_SHORTCUTS
#undef X
        SHORTCUT_COUNT
  };

  constexpr const char* SHORTCUT_STRINGS[] = {
#define X(symbol, name, string, type, ...) string,
      SETTINGS_SHORTCUTS
#undef X
  };

  using ShortcutMember = std::string Settings::*;
  constexpr ShortcutMember SHORTCUT_MEMBERS[] = {
#define X(symbol, name, string, type, ...) &Settings::name,
      SETTINGS_SHORTCUTS
#undef X
  };

  enum WindowType
  {
#define X(symbol, name, string, type, ...) symbol,
    SETTINGS_WINDOWS
#undef X
        WINDOW_COUNT
  };

  constexpr const char* WINDOW_STRINGS[] = {
#define X(symbol, name, string, type, ...) string,
      SETTINGS_WINDOWS
#undef X
  };

  using WindowMember = bool Settings::*;
  static constexpr WindowMember WINDOW_MEMBERS[] = {
#define X(symbol, name, string, type, ...) &Settings::name,
      SETTINGS_WINDOWS
#undef X
  };
}
