#pragma once
#include "embed.h"
#include <glm/ext/vector_int2.hpp>

  constexpr auto ICON_SIZE_SMALL = glm::ivec2(64, 64);
  constexpr auto ICON_SIZE_NORMAL = glm::ivec2(128, 128);
  constexpr auto ICON_SIZE_LARGE = glm::ivec2(256, 256);
  constexpr auto ICON_SIZE_HUGE = glm::ivec2(512, 512);

#define SVG_LIST                                                                                                       \
  X(ICON_NONE, &ICON_NONE_SVG, ICON_SIZE_SMALL)                                                                                       \
  X(ICON_FILE, &ICON_FILE_SVG, ICON_SIZE_NORMAL)                                                                                      \
  X(ICON_FOLDER, &ICON_FOLDER_SVG, ICON_SIZE_NORMAL)                                                                                  \
  X(ICON_FOLDER_OPEN, &ICON_FOLDER_OPEN_SVG, ICON_SIZE_NORMAL)                                                                        \
  X(ICON_CLOSE, &ICON_CLOSE_SVG, ICON_SIZE_NORMAL)                                                                                    \
  X(ICON_ROOT, &ICON_ROOT_SVG, ICON_SIZE_NORMAL)                                                                                      \
  X(ICON_LAYER, &ICON_LAYER_SVG, ICON_SIZE_NORMAL)                                                                                    \
  X(ICON_NULL, &ICON_NULL_SVG, ICON_SIZE_NORMAL)                                                                                     \
  X(ICON_SOUND, &ICON_SOUND_SVG, ICON_SIZE_NORMAL)                                                                                    \
  X(ICON_SHADER, &ICON_SHADER_SVG, ICON_SIZE_NORMAL)                                                                                  \
  X(ICON_TRIGGERS, &ICON_TRIGGERS_SVG, ICON_SIZE_NORMAL)                                                                              \
  X(ICON_VISIBLE, &ICON_VISIBLE_SVG, ICON_SIZE_NORMAL)                                                                                \
  X(ICON_INVISIBLE, &ICON_INVISIBLE_SVG, ICON_SIZE_NORMAL)                                                                            \
  X(ICON_SHOW_RECT, &ICON_SHOW_RECT_SVG, ICON_SIZE_NORMAL)                                                                            \
  X(ICON_HIDE_RECT, &ICON_HIDE_RECT_SVG, ICON_SIZE_NORMAL)                                                                            \
  X(ICON_SHOW_UNUSED, &ICON_SHOW_UNUSED_SVG, ICON_SIZE_NORMAL)                                                                        \
  X(ICON_HIDE_UNUSED, &ICON_HIDE_UNUSED_SVG, ICON_SIZE_NORMAL)                                                                        \
  X(ICON_SHOW_LAYERS, &ICON_SHOW_LAYERS_SVG, ICON_SIZE_NORMAL)                                                                        \
  X(ICON_HIDE_LAYERS, &ICON_HIDE_LAYERS_SVG, ICON_SIZE_NORMAL)                                                                        \
  X(ICON_OVER, &ICON_OVER_SVG, ICON_SIZE_NORMAL)                                                                                      \
  X(ICON_UNDER, &ICON_UNDER_SVG, ICON_SIZE_NORMAL)                                                                                    \
  X(ICON_PAN, &ICON_PAN_SVG, ICON_SIZE_NORMAL)                                                                                        \
  X(ICON_MOVE, &ICON_MOVE_SVG, ICON_SIZE_NORMAL)                                                                                      \
  X(ICON_ROTATE, &ICON_ROTATE_SVG, ICON_SIZE_NORMAL)                                                                                  \
  X(ICON_SCALE, &ICON_SCALE_SVG, ICON_SIZE_NORMAL)                                                                                    \
  X(ICON_SKEW, &ICON_SKEW_SVG, ICON_SIZE_NORMAL)                                                                                    \
  X(ICON_CROP, &ICON_CROP_SVG, ICON_SIZE_NORMAL)                                                                                      \
  X(ICON_DRAW, &ICON_DRAW_SVG, ICON_SIZE_NORMAL)                                                                                      \
  X(ICON_ERASE, &ICON_ERASE_SVG, ICON_SIZE_NORMAL)                                                                                    \
  X(ICON_COLOR_PICKER, &ICON_COLOR_PICKER_SVG, ICON_SIZE_NORMAL)                                                                      \
  X(ICON_UNDO, &ICON_UNDO_SVG, ICON_SIZE_NORMAL)                                                                                      \
  X(ICON_REDO, &ICON_REDO_SVG, ICON_SIZE_NORMAL)                                                                                      \
  X(ICON_ANIMATION, &ICON_ANIMATION_SVG, ICON_SIZE_NORMAL)                                                                            \
  X(ICON_SPRITESHEET, &ICON_SPRITESHEET_SVG, ICON_SIZE_NORMAL)                                                                        \
  X(ICON_EVENT, &ICON_EVENT_SVG, ICON_SIZE_NORMAL)                                                                                    \
  X(ICON_INTERPOLATED, &ICON_INTERPOLATED_SVG, ICON_SIZE_NORMAL)                                                                      \
  X(ICON_UNINTERPOLATED, &ICON_UNINTERPOLATED_SVG, ICON_SIZE_NORMAL)                                                                  \
  X(ICON_EASE_IN, &ICON_EASE_IN_SVG, ICON_SIZE_NORMAL)                                                                                \
  X(ICON_EASE_OUT, &ICON_EASE_OUT_SVG, ICON_SIZE_NORMAL)                                                                              \
  X(ICON_EASE_IN_OUT, &ICON_EASE_IN_OUT_SVG, ICON_SIZE_NORMAL)                                                                        \
  X(ICON_TRIGGER, &ICON_TRIGGER_SVG, ICON_SIZE_NORMAL)                                                                                \
  X(ICON_PLAYHEAD, &ICON_PLAYHEAD_SVG, ICON_SIZE_NORMAL)                                                                              \
  X(ICON_PIVOT, &ICON_PIVOT_SVG, ICON_SIZE_NORMAL)                                                                                    \
  X(ICON_POINT, &ICON_UNINTERPOLATED_SVG, ICON_SIZE_NORMAL)                                                                           \
  X(ICON_TARGET, &ICON_TARGET_SVG, ICON_SIZE_HUGE)                                                                                    \
  X(ICON_TARGET_ALT, &ICON_TARGET_ALT_SVG, ICON_SIZE_HUGE)

  enum Icon
  {
#define X(name, data, size) name,
    SVG_LIST
#undef X
        ICON_COUNT
  };

  struct IconInfo
  {
    const Embed* svg{};
    glm::ivec2 size{};
  };

  constexpr IconInfo ICONS[ICON_COUNT] = {
#define X(name, data, size) {data, size},
      SVG_LIST
#undef X
  };
