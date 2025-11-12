#pragma once

#include <optional>
#include <string>
#include <tinyxml2/tinyxml2.h>

#include "anm2_type.h"
#include "types.h"

#if defined(TRANSPARENT)
  #undef TRANSPARENT
#endif

namespace anm2ed::anm2
{
  constexpr auto FRAME_DURATION_MIN = 1;
  constexpr auto FRAME_DURATION_MAX = 100000;

#define MEMBERS                                                                                                        \
  X(isVisible, bool, true)                                                                                             \
  X(isInterpolated, bool, false)                                                                                       \
  X(rotation, float, 0.0f)                                                                                             \
  X(duration, int, FRAME_DURATION_MIN)                                                                                 \
  X(atFrame, int, -1)                                                                                                  \
  X(eventID, int, -1)                                                                                                  \
  X(soundID, int, -1)                                                                                                  \
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
    bool is_visible(Type = NONE);
  };

  struct FrameChange
  {
#define X(name, type, ...) std::optional<type> name{};
    MEMBERS
#undef X
  };

#undef MEMBERS
}
