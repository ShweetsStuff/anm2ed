#pragma once

#include "icon.h"

#include <glm/glm/vec2.hpp>
#include <glm/glm/vec3.hpp>
#include <glm/glm/vec4.hpp>

namespace anm2ed::anm2
{
  inline const glm::vec4 ROOT_COLOR = glm::vec4(0.140f, 0.310f, 0.560f, 1.000f);
  inline const glm::vec4 ROOT_COLOR_ACTIVE = glm::vec4(0.240f, 0.520f, 0.880f, 1.000f);
  inline const glm::vec4 ROOT_COLOR_HOVERED = glm::vec4(0.320f, 0.640f, 1.000f, 1.000f);

  inline const glm::vec4 LAYER_COLOR = glm::vec4(0.640f, 0.320f, 0.110f, 1.000f);
  inline const glm::vec4 LAYER_COLOR_ACTIVE = glm::vec4(0.840f, 0.450f, 0.170f, 1.000f);
  inline const glm::vec4 LAYER_COLOR_HOVERED = glm::vec4(0.960f, 0.560f, 0.240f, 1.000f);

  inline const glm::vec4 NULL_COLOR = glm::vec4(0.140f, 0.430f, 0.200f, 1.000f);
  inline const glm::vec4 NULL_COLOR_ACTIVE = glm::vec4(0.250f, 0.650f, 0.350f, 1.000f);
  inline const glm::vec4 NULL_COLOR_HOVERED = glm::vec4(0.350f, 0.800f, 0.480f, 1.000f);

  inline const glm::vec4 TRIGGER_COLOR = glm::vec4(0.620f, 0.150f, 0.260f, 1.000f);
  inline const glm::vec4 TRIGGER_COLOR_ACTIVE = glm::vec4(0.820f, 0.250f, 0.380f, 1.000f);
  inline const glm::vec4 TRIGGER_COLOR_HOVERED = glm::vec4(0.950f, 0.330f, 0.490f, 1.000f);

#define TYPE_LIST                                                                                                      \
  X(NONE, "", "", resource::icon::NONE, glm::vec4(), glm::vec4(), glm::vec4())                                         \
  X(ROOT, "Root", "RootAnimation", resource::icon::ROOT, ROOT_COLOR, ROOT_COLOR_ACTIVE, ROOT_COLOR_HOVERED)            \
  X(LAYER, "Layer", "LayerAnimation", resource::icon::LAYER, LAYER_COLOR, LAYER_COLOR_ACTIVE, LAYER_COLOR_HOVERED)     \
  X(NULL_, "Null", "NullAnimation", resource::icon::NULL_, NULL_COLOR, NULL_COLOR_ACTIVE, NULL_COLOR_HOVERED)          \
  X(TRIGGER, "Trigger", "Triggers", resource::icon::TRIGGERS, TRIGGER_COLOR, TRIGGER_COLOR_ACTIVE,                     \
    TRIGGER_COLOR_HOVERED)

  enum Type
  {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) symbol,
    TYPE_LIST
#undef X
  };

  constexpr const char* TYPE_STRINGS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) string,
      TYPE_LIST
#undef X
  };

  constexpr const char* TYPE_ITEM_STRINGS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) itemString,
      TYPE_LIST
#undef X
  };

  constexpr resource::icon::Type TYPE_ICONS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) icon,
      TYPE_LIST
#undef X
  };

  inline const glm::vec4 TYPE_COLOR[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) color,
      TYPE_LIST
#undef X
  };

  inline const glm::vec4 TYPE_COLOR_ACTIVE[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) colorActive,
      TYPE_LIST
#undef X
  };

  inline const glm::vec4 TYPE_COLOR_HOVERED[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) colorHovered,
      TYPE_LIST
#undef X
  };

  enum ChangeType
  {
    ADJUST,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
  };
}
