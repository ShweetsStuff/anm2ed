#pragma once

#include <optional>
#include <string>
#include <tinyxml2/tinyxml2.h>

#include "anm2_type.hpp"
#include "types.hpp"

namespace anm2ed::anm2
{
  constexpr auto FRAME_DURATION_MIN = 1;
  constexpr auto FRAME_DURATION_MAX = 1000000;

  class Frame
  {
  public:
    enum Interpolation
    {
      NONE,
      LINEAR,
      EASE_IN,
      EASE_OUT,
      EASE_IN_OUT
    };

    bool isVisible{true};
    Interpolation interpolation{NONE};
    float rotation{};
    int duration{FRAME_DURATION_MIN};
    int atFrame{-1};
    int eventID{-1};
    int regionID{-1};
    std::vector<int> soundIDs{};
    glm::vec2 pivot{};
    glm::vec2 crop{};
    glm::vec2 position{};
    glm::vec2 size{};
    glm::vec2 scale{100, 100};
    glm::vec3 colorOffset{};
    glm::vec4 tint{types::color::WHITE};

    Frame() = default;
    Frame(tinyxml2::XMLElement*, Type);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, Type, Flags = 0);
    std::string to_string(Type type, Flags = 0);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, Type, Flags = 0);
    void shorten();
    void extend();
  };

  struct FrameChange
  {
    std::optional<bool> isVisible{};
    std::optional<Frame::Interpolation> interpolation{};
    std::optional<float> rotation{};
    std::optional<int> duration{};
    std::optional<int> regionID{};
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
    std::optional<bool> isFlipX{};
    std::optional<bool> isFlipY{};
  };

}
