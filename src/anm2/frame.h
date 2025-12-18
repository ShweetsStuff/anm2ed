#pragma once

#include <optional>
#include <string>
#include <tinyxml2/tinyxml2.h>

#include "anm2_type.h"
#include "types.h"

namespace anm2ed::anm2
{
  constexpr auto FRAME_DURATION_MIN = 1;
  constexpr auto FRAME_DURATION_MAX = 1000000;

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
  X(colorOffset, glm::vec3, glm::vec3())                                                                               \
  X(tint, glm::vec4, types::color::WHITE)

  class Frame
  {
  public:
#define X(name, type, ...) type name = __VA_ARGS__;
    MEMBERS
#undef X

#undef MEMBERS

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
    std::optional<bool> isVisible{};
    std::optional<bool> isInterpolated{};
    std::optional<float> rotation{};
    std::optional<int> duration{};
    std::optional<int> atFrame{};
    std::optional<int> eventID{};
    std::optional<float> pivotX{};
    std::optional<float> pivotY{};
    std::optional<float> cropX{};
    std::optional<float> cropY{};
    std::optional<float> positionX{};
    std::optional<float> positionY{};
    std::optional<float> sizeX{};
    std::optional<float> sizeY{};
    std::optional<float> scaleX{};
    std::optional<float> scaleY{};
    std::optional<float> colorOffsetR{};
    std::optional<float> colorOffsetG{};
    std::optional<float> colorOffsetB{};
    std::optional<float> tintR{};
    std::optional<float> tintG{};
    std::optional<float> tintB{};
    std::optional<float> tintA{};
  };

}
