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

  class Frame
  {
  public:
    bool isVisible{true};
    bool isInterpolated{false};
    float rotation{};
    int duration{FRAME_DURATION_MIN};
    int atFrame{-1};
    int eventID{-1};
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
