#pragma once

#include <optional>
#include <string>
#include <tinyxml2/tinyxml2.h>

#include <glm/glm/vec2.hpp>
#include <glm/glm/vec3.hpp>
#include <glm/glm/vec4.hpp>

#include "types.h"

namespace anm2ed::anm2
{
  constexpr auto FRAME_DELAY_MIN = 1;
  constexpr auto FRAME_DELAY_MAX = 100000;

#define TYPE_LIST                                                                                                      \
  X(NONE, "None", "None")                                                                                              \
  X(ROOT, "Root", "RootAnimation")                                                                                     \
  X(LAYER, "Layer", "LayerAnimation")                                                                                  \
  X(NULL_, "Null", "NullAnimation")                                                                                    \
  X(TRIGGER, "Trigger", "Triggers")

  enum Type
  {
#define X(symbol, string, animationString) symbol,
    TYPE_LIST
#undef X
  };

  constexpr const char* TYPE_STRINGS[] = {
#define X(symbol, string, animationString) string,
      TYPE_LIST
#undef X
  };

  constexpr const char* TYPE_ANIMATION_STRINGS[] = {
#define X(symbol, string, animationString) animationString,
      TYPE_LIST
#undef X
  };

  enum ChangeType
  {
    ADD,
    SUBTRACT,
    ADJUST
  };

#define MEMBERS                                                                                                        \
  X(isVisible, bool, true)                                                                                             \
  X(isInterpolated, bool, false)                                                                                       \
  X(rotation, float, 0.0f)                                                                                             \
  X(delay, int, FRAME_DELAY_MIN)                                                                                       \
  X(atFrame, int, -1)                                                                                                  \
  X(eventID, int, -1)                                                                                                  \
  X(pivot, glm::vec2, {})                                                                                              \
  X(crop, glm::vec2, {})                                                                                               \
  X(position, glm::vec2, {})                                                                                           \
  X(size, glm::vec2, {})                                                                                               \
  X(scale, glm::vec2, glm::vec2(100.0f))                                                                               \
  X(colorOffset, glm::vec3, types::color::TRANSPARENT)                                                                 \
  X(tint, glm::vec4, types::color::WHITE)

  class Frame
  {
  public:
#define X(name, type, ...) type name = __VA_ARGS__;
    MEMBERS
#undef X

    Frame() = default;
    Frame(tinyxml2::XMLElement*, Type);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, Type);
    std::string to_string(Type type);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, Type);
    void shorten();
    void extend();
  };

  struct FrameChange
  {
#define X(name, type, ...) std::optional<type> name{};
    MEMBERS
#undef X
  };

#undef MEMBERS

}