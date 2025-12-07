#pragma once

#include <string>

#include <glm/glm.hpp>

#include "anm2/anm2_type.h"
#include "render.h"
#include "strings.h"
#include "types.h"

namespace anm2ed
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
  X(WINDOW_SIZE, windowSize, STRING_UNDEFINED, IVEC2_WH, {1600, 900})                                                  \
  X(IS_VSYNC, isVsync, STRING_UNDEFINED, BOOL, true)                                                                   \
  X(UI_SCALE, uiScale, STRING_UNDEFINED, FLOAT, 1.0f)                                                                  \
  X(THEME, theme, STRING_UNDEFINED, INT, types::theme::DARK)                                                           \
  X(LANGUAGE, language, STRING_UNDEFINED, INT, ENGLISH)                                                                \
                                                                                                                       \
  X(FILE_IS_AUTOSAVE, fileIsAutosave, STRING_UNDEFINED, BOOL, true)                                                    \
  X(FILE_AUTOSAVE_TIME, fileAutosaveTime, STRING_UNDEFINED, INT, 1)                                                    \
  X(FILE_IS_WARN_OVERWRITE, fileIsWarnOverwrite, STRING_UNDEFINED, BOOL, true)                                         \
  X(FILE_SNAPSHOT_STACK_SIZE, fileSnapshotStackSize, STRING_UNDEFINED, INT, 50)                                        \
                                                                                                                       \
  X(KEYBOARD_REPEAT_DELAY, keyboardRepeatDelay, STRING_UNDEFINED, FLOAT, 0.300f)                                       \
  X(KEYBOARD_REPEAT_RATE, keyboardRepeatRate, STRING_UNDEFINED, FLOAT, 0.050f)                                         \
                                                                                                                       \
  X(INPUT_ZOOM_STEP, inputZoomStep, STRING_UNDEFINED, FLOAT, 50.0f)                                                    \
  X(INPUT_IS_MOVE_TOOL_SNAP_TO_MOUSE, inputIsMoveToolSnapToMouse, STRING_UNDEFINED, BOOL, false)                       \
                                                                                                                       \
  X(PLAYBACK_IS_LOOP, playbackIsLoop, STRING_UNDEFINED, BOOL, true)                                                    \
  X(PLAYBACK_IS_CLAMP, playbackIsClamp, STRING_UNDEFINED, BOOL, true)                                                  \
                                                                                                                       \
  X(CHANGE_IS_CROP, changeIsCrop, STRING_UNDEFINED, BOOL, false)                                                       \
  X(CHANGE_IS_SIZE, changeIsSize, STRING_UNDEFINED, BOOL, false)                                                       \
  X(CHANGE_IS_POSITION, changeIsPosition, STRING_UNDEFINED, BOOL, false)                                               \
  X(CHANGE_IS_PIVOT, changeIsPivot, STRING_UNDEFINED, BOOL, false)                                                     \
  X(CHANGE_IS_SCALE, changeIsScale, STRING_UNDEFINED, BOOL, false)                                                     \
  X(CHANGE_IS_ROTATION, changeIsRotation, STRING_UNDEFINED, BOOL, false)                                               \
  X(CHANGE_IS_DURATION, changeIsDuration, STRING_UNDEFINED, BOOL, false)                                               \
  X(CHANGE_IS_TINT, changeIsTint, STRING_UNDEFINED, BOOL, false)                                                       \
  X(CHANGE_IS_COLOR_OFFSET, changeIsColorOffset, STRING_UNDEFINED, BOOL, false)                                        \
  X(CHANGE_IS_VISIBLE_SET, changeIsVisibleSet, STRING_UNDEFINED, BOOL, false)                                          \
  X(CHANGE_IS_INTERPOLATED_SET, changeIsInterpolatedSet, STRING_UNDEFINED, BOOL, false)                                \
  X(CHANGE_CROP, changeCrop, STRING_UNDEFINED, VEC2, {})                                                               \
  X(CHANGE_SIZE, changeSize, STRING_UNDEFINED, VEC2, {})                                                               \
  X(CHANGE_POSITION, changePosition, STRING_UNDEFINED, VEC2, {})                                                       \
  X(CHANGE_PIVOT, changePivot, STRING_UNDEFINED, VEC2, {})                                                             \
  X(CHANGE_SCALE, changeScale, STRING_UNDEFINED, VEC2, {})                                                             \
  X(CHANGE_ROTATION, changeRotation, STRING_UNDEFINED, FLOAT, 0.0f)                                                    \
  X(CHANGE_DURATION, changeDuration, STRING_UNDEFINED, INT, 0)                                                         \
  X(CHANGE_TINT, changeTint, STRING_UNDEFINED, VEC4, {})                                                               \
  X(CHANGE_COLOR_OFFSET, changeColorOffset, STRING_UNDEFINED, VEC3, {})                                                \
  X(CHANGE_IS_VISIBLE, changeIsVisible, STRING_UNDEFINED, BOOL, false)                                                 \
  X(CHANGE_IS_INTERPOLATED, changeIsInterpolated, STRING_UNDEFINED, BOOL, false)                                       \
  X(CHANGE_NUMBER_FRAMES, changeNumberFrames, STRING_UNDEFINED, INT, 1)                                                \
  X(CHANGE_IS_FROM_SELECTED_FRAME, changeIsFromSelectedFrame, STRING_UNDEFINED, BOOL, false)                           \
                                                                                                                       \
  X(SCALE_VALUE, scaleValue, STRING_UNDEFINED, FLOAT, 1.0f)                                                            \
                                                                                                                       \
  X(PREVIEW_IS_AXES, previewIsAxes, STRING_UNDEFINED, BOOL, true)                                                      \
  X(PREVIEW_IS_GRID, previewIsGrid, STRING_UNDEFINED, BOOL, true)                                                      \
  X(PREVIEW_IS_ROOT_TRANSFORM, previewIsRootTransform, STRING_UNDEFINED, BOOL, true)                                   \
  X(PREVIEW_IS_PIVOTS, previewIsPivots, STRING_UNDEFINED, BOOL, false)                                                 \
  X(PREVIEW_IS_BORDER, previewIsBorder, STRING_UNDEFINED, BOOL, false)                                                 \
  X(PREVIEW_IS_ALT_ICONS, previewIsAltIcons, STRING_UNDEFINED, BOOL, false)                                            \
  X(PREVIEW_OVERLAY_TRANSPARENCY, previewOverlayTransparency, STRING_UNDEFINED, FLOAT, 255)                            \
  X(PREVIEW_START_ZOOM, previewStartZoom, STRING_UNDEFINED, FLOAT, 200.0f)                                             \
  X(PREVIEW_GRID_SIZE, previewGridSize, STRING_UNDEFINED, IVEC2, {32, 32})                                             \
  X(PREVIEW_GRID_OFFSET, previewGridOffset, STRING_UNDEFINED, IVEC2, {})                                               \
  X(PREVIEW_GRID_COLOR, previewGridColor, STRING_UNDEFINED, VEC4, {1.0f, 1.0f, 1.0f, 0.125f})                          \
  X(PREVIEW_AXES_COLOR, previewAxesColor, STRING_UNDEFINED, VEC4, {1.0f, 1.0f, 1.0f, 0.125f})                          \
  X(PREVIEW_BACKGROUND_COLOR, previewBackgroundColor, STRING_UNDEFINED, VEC3, {0.113f, 0.184f, 0.286f})                \
                                                                                                                       \
  X(PROPERTIES_IS_ROUND, propertiesIsRound, STRING_UNDEFINED, BOOL, false)                                             \
                                                                                                                       \
  X(GENERATE_START_POSITION, generateStartPosition, STRING_UNDEFINED, IVEC2, {})                                       \
  X(GENERATE_SIZE, generateSize, STRING_UNDEFINED, IVEC2, {64, 64})                                                    \
  X(GENERATE_PIVOT, generatePivot, STRING_UNDEFINED, IVEC2, {32, 32})                                                  \
  X(GENERATE_ROWS, generateRows, STRING_UNDEFINED, INT, 4)                                                             \
  X(GENERATE_COLUMNS, generateColumns, STRING_UNDEFINED, INT, 4)                                                       \
  X(GENERATE_COUNT, generateCount, STRING_UNDEFINED, INT, 16)                                                          \
  X(GENERATE_DURATION, generateDuration, STRING_UNDEFINED, INT, 1)                                                     \
  X(GENERATE_ZOOM, generateZoom, STRING_UNDEFINED, FLOAT, 100.0f)                                                      \
                                                                                                                       \
  X(EDITOR_IS_GRID, editorIsGrid, STRING_UNDEFINED, BOOL, true)                                                        \
  X(EDITOR_IS_GRID_SNAP, editorIsGridSnap, STRING_UNDEFINED, BOOL, true)                                               \
  X(EDITOR_IS_BORDER, editorIsBorder, STRING_UNDEFINED, BOOL, true)                                                    \
  X(EDITOR_IS_TRANSPARENT, editorIsTransparent, STRING_UNDEFINED, BOOL, true)                                          \
  X(EDITOR_START_ZOOM, editorStartZoom, STRING_UNDEFINED, FLOAT, 200.0f)                                               \
  X(EDITOR_SIZE, editorSize, STRING_UNDEFINED, IVEC2_WH, {1200, 600})                                                  \
  X(EDITOR_GRID_SIZE, editorGridSize, STRING_UNDEFINED, IVEC2, {32, 32})                                               \
  X(EDITOR_GRID_OFFSET, editorGridOffset, STRING_UNDEFINED, IVEC2, {32, 32})                                           \
  X(EDITOR_GRID_COLOR, editorGridColor, STRING_UNDEFINED, VEC4, {1.0, 1.0, 1.0, 0.125})                                \
  X(EDITOR_BACKGROUND_COLOR, editorBackgroundColor, STRING_UNDEFINED, VEC3, {0.113, 0.184, 0.286})                     \
                                                                                                                       \
  X(MERGE_TYPE, mergeType, STRING_UNDEFINED, INT, 0)                                                                   \
  X(MERGE_IS_DELETE_ANIMATIONS_AFTER, mergeIsDeleteAnimationsAfter, STRING_UNDEFINED, BOOL, false)                     \
                                                                                                                       \
  X(BAKE_INTERVAL, bakeInterval, STRING_UNDEFINED, INT, 1)                                                             \
  X(BAKE_IS_ROUND_SCALE, bakeIsRoundScale, STRING_UNDEFINED, BOOL, true)                                               \
  X(BAKE_IS_ROUND_ROTATION, bakeIsRoundRotation, STRING_UNDEFINED, BOOL, true)                                         \
                                                                                                                       \
  X(TIMELINE_ADD_ITEM_TYPE, timelineAddItemType, STRING_UNDEFINED, INT, anm2::LAYER)                                   \
  X(TIMELINE_ADD_ITEM_DESTINATION, timelineAddItemDestination, STRING_UNDEFINED, INT, types::destination::ALL)         \
  X(TIMELINE_ADD_ITEM_SOURCE, timelineAddItemSource, STRING_UNDEFINED, INT, types::source::NEW)                        \
  X(TIMELINE_IS_SHOW_UNUSED, timelineIsShowUnused, STRING_UNDEFINED, BOOL, true)                                       \
  X(TIMELINE_IS_ONLY_SHOW_LAYERS, timelineIsOnlyShowLayers, STRING_UNDEFINED, BOOL, false)                             \
  X(TIMELINE_IS_SOUND, timelineIsSound, STRING_UNDEFINED, BOOL, true)                                                  \
                                                                                                                       \
  X(ONIONSKIN_IS_ENABLED, onionskinIsEnabled, STRING_UNDEFINED, BOOL, false)                                           \
  X(ONIONSKIN_BEFORE_COUNT, onionskinBeforeCount, STRING_UNDEFINED, INT, 0)                                            \
  X(ONIONSKIN_AFTER_COUNT, onionskinAfterCount, STRING_UNDEFINED, INT, 0)                                              \
  X(ONIONSKIN_BEFORE_COLOR, onionskinBeforeColor, STRING_UNDEFINED, VEC3, types::color::RED)                           \
  X(ONIONSKIN_AFTER_COLOR, onionskinAfterColor, STRING_UNDEFINED, VEC3, types::color::BLUE)                            \
  X(ONIONSKIN_MODE, onionskinMode, STRING_UNDEFINED, INT, (int)types::OnionskinMode::TIME)                             \
                                                                                                                       \
  X(TOOL, tool, STRING_UNDEFINED, INT, 0)                                                                              \
  X(TOOL_COLOR, toolColor, STRING_UNDEFINED, VEC4, {1.0, 1.0, 1.0, 1.0})                                               \
                                                                                                                       \
  X(RENDER_TYPE, renderType, STRING_UNDEFINED, INT, render::GIF)                                                       \
  X(RENDER_PATH, renderPath, STRING_UNDEFINED, STRING, "./output.gif")                                                 \
  X(RENDER_ROWS, renderRows, STRING_UNDEFINED, INT, 0)                                                                 \
  X(RENDER_COLUMNS, renderColumns, STRING_UNDEFINED, INT, 0)                                                           \
  X(RENDER_FORMAT, renderFormat, STRING_UNDEFINED, STRING, "{}.png")                                                   \
  X(RENDER_IS_RAW_ANIMATION, renderIsRawAnimation, STRING_UNDEFINED, BOOL, true)                                       \
  X(RENDER_SCALE, renderScale, STRING_UNDEFINED, FLOAT, 1.0f)                                                          \
  X(RENDER_FFMPEG_PATH, renderFFmpegPath, STRING_UNDEFINED, STRING, FFMPEG_PATH_DEFAULT)

#define SETTINGS_SHORTCUTS                                                                                             \
  /* Symbol / Name / String / Type / Default */                                                                        \
  /* File */                                                                                                           \
  X(SHORTCUT_NEW, shortcutNew, SHORTCUT_STRING_NEW, STRING, "Ctrl+N")                                                  \
  X(SHORTCUT_OPEN, shortcutOpen, SHORTCUT_STRING_OPEN, STRING, "Ctrl+O")                                               \
  X(SHORTCUT_CLOSE, shortcutClose, SHORTCUT_STRING_CLOSE, STRING, "Ctrl+W")                                            \
  X(SHORTCUT_SAVE, shortcutSave, SHORTCUT_STRING_SAVE, STRING, "Ctrl+S")                                               \
  X(SHORTCUT_SAVE_AS, shortcutSaveAs, SHORTCUT_STRING_SAVE_AS, STRING, "Ctrl+Shift+S")                                 \
  X(SHORTCUT_EXIT, shortcutExit, SHORTCUT_STRING_EXIT, STRING, "Alt+F4")                                               \
  /* Edit */                                                                                                           \
  X(SHORTCUT_UNDO, shortcutUndo, SHORTCUT_STRING_UNDO, STRING, "Ctrl+Z")                                               \
  X(SHORTCUT_REDO, shortcutRedo, SHORTCUT_STRING_REDO, STRING, "Ctrl+Y")                                               \
  X(SHORTCUT_CUT, shortcutCut, SHORTCUT_STRING_CUT, STRING, "Ctrl+X")                                                  \
  X(SHORTCUT_COPY, shortcutCopy, SHORTCUT_STRING_COPY, STRING, "Ctrl+C")                                               \
  X(SHORTCUT_PASTE, shortcutPaste, SHORTCUT_STRING_PASTE, STRING, "Ctrl+V")                                            \
  X(SHORTCUT_DUPLICATE, shortcutDuplicate, SHORTCUT_STRING_DUPLICATE, STRING, "Ctrl+J")                                \
  X(SHORTCUT_ADD, shortcutAdd, SHORTCUT_STRING_ADD, STRING, "Insert")                                                  \
  X(SHORTCUT_REMOVE, shortcutRemove, SHORTCUT_STRING_REMOVE, STRING, "Delete")                                         \
  X(SHORTCUT_DEFAULT, shortcutDefault, SHORTCUT_STRING_DEFAULT, STRING, "Home")                                        \
  X(SHORTCUT_MERGE, shortcutMerge, SHORTCUT_STRING_MERGE, STRING, "Ctrl+E")                                            \
  /* Tools */                                                                                                          \
  X(SHORTCUT_PAN, shortcutPan, SHORTCUT_STRING_PAN, STRING, "P")                                                       \
  X(SHORTCUT_MOVE, shortcutMove, SHORTCUT_STRING_MOVE, STRING, "V")                                                    \
  X(SHORTCUT_ROTATE, shortcutRotate, SHORTCUT_STRING_ROTATE, STRING, "R")                                              \
  X(SHORTCUT_SCALE, shortcutScale, SHORTCUT_STRING_SCALE, STRING, "S")                                                 \
  X(SHORTCUT_CROP, shortcutCrop, SHORTCUT_STRING_CROP, STRING, "C")                                                    \
  X(SHORTCUT_DRAW, shortcutDraw, SHORTCUT_STRING_DRAW, STRING, "B")                                                    \
  X(SHORTCUT_ERASE, shortcutErase, SHORTCUT_STRING_ERASE, STRING, "E")                                                 \
  X(SHORTCUT_COLOR_PICKER, shortcutColorPicker, SHORTCUT_STRING_COLOR_PICKER, STRING, "I")                             \
  X(SHORTCUT_COLOR, shortcutColor, SHORTCUT_STRING_COLOR, STRING, "X")                                                 \
  /* View */                                                                                                           \
  X(SHORTCUT_CENTER_VIEW, shortcutCenterView, SHORTCUT_STRING_CENTER_VIEW, STRING, "Home")                             \
  X(SHORTCUT_FIT, shortcutFit, SHORTCUT_STRING_FIT, STRING, "F")                                                       \
  X(SHORTCUT_ZOOM_IN, shortcutZoomIn, SHORTCUT_STRING_ZOOM_IN, STRING, "Ctrl+Equal")                                   \
  X(SHORTCUT_ZOOM_OUT, shortcutZoomOut, SHORTCUT_STRING_ZOOM_OUT, STRING, "Ctrl+Minus")                                \
  /* Timeline / Playback */                                                                                            \
  X(SHORTCUT_BAKE, shortcutBake, SHORTCUT_STRING_BAKE, STRING, "Ctrl+B")                                               \
  X(SHORTCUT_SPLIT, shortcutSplit, SHORTCUT_STRING_SPLIT, STRING, "Ctrl+P")                                            \
  X(SHORTCUT_PLAY_PAUSE, shortcutPlayPause, SHORTCUT_STRING_PLAY_PAUSE, STRING, "Space")                               \
  X(SHORTCUT_MOVE_PLAYHEAD_BACK, shortcutMovePlayheadBack, SHORTCUT_STRING_PLAYHEAD_BACK, STRING, "Comma")             \
  X(SHORTCUT_MOVE_PLAYHEAD_FORWARD, shortcutMovePlayheadForward, SHORTCUT_STRING_PLAYHEAD_FORWARD, STRING, "Period")   \
  X(SHORTCUT_SHORTEN_FRAME, shortcutShortenFrame, SHORTCUT_STRING_SHORTEN_FRAME, STRING, "F4")                         \
  X(SHORTCUT_EXTEND_FRAME, shortcutExtendFrame, SHORTCUT_STRING_EXTEND_FRAME, STRING, "F5")                            \
  X(SHORTCUT_INSERT_FRAME, shortcutInsertFrame, SHORTCUT_STRING_INSERT_FRAME, STRING, "F6")                            \
  X(SHORTCUT_PREVIOUS_FRAME, shortcutPreviousFrame, SHORTCUT_STRING_PREVIOUS_FRAME, STRING, "F7")                      \
  X(SHORTCUT_NEXT_FRAME, shortcutNextFrame, SHORTCUT_STRING_NEXT_FRAME, STRING, "F8")                                  \
  /* Toggles */                                                                                                        \
  X(SHORTCUT_ONIONSKIN, shortcutOnionskin, SHORTCUT_STRING_ONIONSKIN, STRING, "O")

#define SETTINGS_WINDOWS                                                                                               \
  /* Symbol / Name / String / Type / Default */                                                                        \
  X(WINDOW_ANIMATIONS, windowIsAnimations, LABEL_ANIMATIONS_WINDOW, BOOL, true)                                        \
  X(WINDOW_ANIMATION_PREVIEW, windowIsAnimationPreview, LABEL_ANIMATION_PREVIEW_WINDOW, BOOL, true)                    \
  X(WINDOW_EVENTS, windowIsEvents, LABEL_EVENTS_WINDOW, BOOL, true)                                                    \
  X(WINDOW_FRAME_PROPERTIES, windowIsFrameProperties, LABEL_FRAME_PROPERTIES_WINDOW, BOOL, true)                       \
  X(WINDOW_LAYERS, windowIsLayers, LABEL_LAYERS_WINDOW, BOOL, true)                                                    \
  X(WINDOW_NULLS, windowIsNulls, LABEL_NULLS_WINDOW, BOOL, true)                                                       \
  X(WINDOW_ONIONSKIN, windowIsOnionskin, LABEL_ONIONSKIN_WINDOW, BOOL, true)                                           \
  X(WINDOW_PREVIEW, windowIsSpritesheets, LABEL_SPRITESHEETS_WINDOW, BOOL, true)                                       \
  X(WINDOW_SOUNDS, windowIsSounds, LABEL_SOUNDS_WINDOW, BOOL, true)                                                    \
  X(WINDOW_SPRITESHEET_EDITOR, windowIsSpritesheetEditor, LABEL_SPRITESHEET_EDITOR_WINDOW, BOOL, true)                 \
  X(WINDOW_TIMELINE, windowIsTimeline, LABEL_TIMELINE_WINDOW, BOOL, true)                                              \
  X(WINDOW_TOOLS, windowIsTools, LABEL_TOOLS_WINDOW, BOOL, true)

  enum ShortcutType
  {
#define X(symbol, name, string, type, ...) symbol,
    SETTINGS_SHORTCUTS
#undef X
        SHORTCUT_COUNT
  };

  class Settings
  {
  public:
#define X(symbol, name, string, type, ...) TYPE_##type name = __VA_ARGS__;
    SETTINGS_MEMBERS SETTINGS_SHORTCUTS SETTINGS_WINDOWS
#undef X

        bool isDefault{};

    Settings() = default;

    Settings(const std::string&);
    void save(const std::string&, const std::string&);
  };

  constexpr StringType SHORTCUT_STRING_TYPES[] = {
#define X(symbol, name, stringType, type, ...) stringType,
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

  constexpr StringType WINDOW_STRING_TYPES[] = {
#define X(symbol, name, stringType, type, ...) stringType,
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
