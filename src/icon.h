#pragma once
#include "embed.h"
#include <glm/ext/vector_int2.hpp>

namespace anm2ed::resource::icon
{
  constexpr auto SIZE_SMALL = glm::ivec2(64, 64);
  constexpr auto SIZE_NORMAL = glm::ivec2(128, 128);
  constexpr auto SIZE_LARGE = glm::ivec2(256, 256);
  constexpr auto SIZE_HUGE = glm::ivec2(512, 512);

#define SVG_LIST                                                                                                       \
  X(NONE, &ICON_NONE_SVG, SIZE_SMALL)                                                                                       \
  X(FILE, &ICON_FILE_SVG, SIZE_NORMAL)                                                                                      \
  X(FOLDER, &ICON_FOLDER_SVG, SIZE_NORMAL)                                                                                  \
  X(FOLDER_OPEN, &ICON_FOLDER_OPEN_SVG, SIZE_NORMAL)                                                                        \
  X(CLOSE, &ICON_CLOSE_SVG, SIZE_NORMAL)                                                                                    \
  X(ROOT, &ICON_ROOT_SVG, SIZE_NORMAL)                                                                                      \
  X(LAYER, &ICON_LAYER_SVG, SIZE_NORMAL)                                                                                    \
  X(NULL_, &ICON_NULL_SVG, SIZE_NORMAL)                                                                                     \
  X(SOUND, &ICON_SOUND_SVG, SIZE_NORMAL)                                                                                    \
  X(SHADER, &ICON_SHADER_SVG, SIZE_NORMAL)                                                                                  \
  X(TRIGGERS, &ICON_TRIGGERS_SVG, SIZE_NORMAL)                                                                              \
  X(VISIBLE, &ICON_VISIBLE_SVG, SIZE_NORMAL)                                                                                \
  X(INVISIBLE, &ICON_INVISIBLE_SVG, SIZE_NORMAL)                                                                            \
  X(SHOW_RECT, &ICON_SHOW_RECT_SVG, SIZE_NORMAL)                                                                            \
  X(HIDE_RECT, &ICON_HIDE_RECT_SVG, SIZE_NORMAL)                                                                            \
  X(SHOW_UNUSED, &ICON_SHOW_UNUSED_SVG, SIZE_NORMAL)                                                                        \
  X(HIDE_UNUSED, &ICON_HIDE_UNUSED_SVG, SIZE_NORMAL)                                                                        \
  X(SHOW_LAYERS, &ICON_SHOW_LAYERS_SVG, SIZE_NORMAL)                                                                        \
  X(HIDE_LAYERS, &ICON_HIDE_LAYERS_SVG, SIZE_NORMAL)                                                                        \
  X(OVER, &ICON_OVER_SVG, SIZE_NORMAL)                                                                                      \
  X(UNDER, &ICON_UNDER_SVG, SIZE_NORMAL)                                                                                    \
  X(PAN, &ICON_PAN_SVG, SIZE_NORMAL)                                                                                        \
  X(MOVE, &ICON_MOVE_SVG, SIZE_NORMAL)                                                                                      \
  X(ROTATE, &ICON_ROTATE_SVG, SIZE_NORMAL)                                                                                  \
  X(SCALE, &ICON_SCALE_SVG, SIZE_NORMAL)                                                                                    \
  X(SKEW, &ICON_SKEW_SVG, SIZE_NORMAL)                                                                                    \
  X(CROP, &ICON_CROP_SVG, SIZE_NORMAL)                                                                                      \
  X(DRAW, &ICON_DRAW_SVG, SIZE_NORMAL)                                                                                      \
  X(ERASE, &ICON_ERASE_SVG, SIZE_NORMAL)                                                                                    \
  X(COLOR_PICKER, &ICON_COLOR_PICKER_SVG, SIZE_NORMAL)                                                                      \
  X(UNDO, &ICON_UNDO_SVG, SIZE_NORMAL)                                                                                      \
  X(REDO, &ICON_REDO_SVG, SIZE_NORMAL)                                                                                      \
  X(ANIMATION, &ICON_ANIMATION_SVG, SIZE_NORMAL)                                                                            \
  X(SPRITESHEET, &ICON_SPRITESHEET_SVG, SIZE_NORMAL)                                                                        \
  X(EVENT, &ICON_EVENT_SVG, SIZE_NORMAL)                                                                                    \
  X(INTERPOLATED, &ICON_INTERPOLATED_SVG, SIZE_NORMAL)                                                                      \
  X(UNINTERPOLATED, &ICON_UNINTERPOLATED_SVG, SIZE_NORMAL)                                                                  \
  X(EASE_IN, &ICON_EASE_IN_SVG, SIZE_NORMAL)                                                                                \
  X(EASE_OUT, &ICON_EASE_OUT_SVG, SIZE_NORMAL)                                                                              \
  X(EASE_IN_OUT, &ICON_EASE_IN_OUT_SVG, SIZE_NORMAL)                                                                        \
  X(TRIGGER, &ICON_TRIGGER_SVG, SIZE_NORMAL)                                                                                \
  X(PLAYHEAD, &ICON_PLAYHEAD_SVG, SIZE_NORMAL)                                                                              \
  X(PIVOT, &ICON_PIVOT_SVG, SIZE_NORMAL)                                                                                    \
  X(POINT, &ICON_UNINTERPOLATED_SVG, SIZE_NORMAL)                                                                           \
  X(TARGET, &ICON_TARGET_SVG, SIZE_HUGE)                                                                                    \
  X(TARGET_ALT, &ICON_TARGET_ALT_SVG, SIZE_HUGE)

  enum Type
  {
#define X(name, data, size) name,
    SVG_LIST
#undef X
        COUNT
  };

  struct Info
  {
    const Embed* svg{};
    glm::ivec2 size{};
  };

  constexpr Info ICONS[COUNT] = {
#define X(name, data, size) {data, size},
      SVG_LIST
#undef X
  };
}
