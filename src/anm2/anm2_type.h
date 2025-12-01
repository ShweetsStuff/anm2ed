#pragma once

#include "icon.h"

#include <glm/glm/vec2.hpp>
#include <glm/glm/vec3.hpp>
#include <glm/glm/vec4.hpp>

namespace anm2ed::anm2
{
  extern const glm::vec4 ROOT_COLOR;
  extern const glm::vec4 ROOT_COLOR_ACTIVE;
  extern const glm::vec4 ROOT_COLOR_HOVERED;

  extern const glm::vec4 LAYER_COLOR;
  extern const glm::vec4 LAYER_COLOR_ACTIVE;
  extern const glm::vec4 LAYER_COLOR_HOVERED;

  extern const glm::vec4 NULL_COLOR;
  extern const glm::vec4 NULL_COLOR_ACTIVE;
  extern const glm::vec4 NULL_COLOR_HOVERED;

  extern const glm::vec4 TRIGGER_COLOR;
  extern const glm::vec4 TRIGGER_COLOR_ACTIVE;
  extern const glm::vec4 TRIGGER_COLOR_HOVERED;

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

  extern const glm::vec4 TYPE_COLOR[];
  extern const glm::vec4 TYPE_COLOR_ACTIVE[];
  extern const glm::vec4 TYPE_COLOR_HOVERED[];

  enum ChangeType
  {
    ADJUST,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
  };
}
