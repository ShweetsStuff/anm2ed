#pragma once

#include <filesystem>
#include <string>
#include <tinyxml2/tinyxml2.h>
#include <vector>

#include "../resource/audio.hpp"
#include "../resource/texture.hpp"

#include <glm/glm.hpp>

namespace anm2ed::resource::anm2_new
{
  class Anm2
  {
  public:
    enum Type
    {
      NONE,
      INFO,
      SPRITESHEET,
      REGION,
      LAYER,
      NULL_,
      EVENT,
      SOUND,
      ANIMATION,
      ROOT_ANIMATION,
      LAYER_ANIMATION,
      NULL_ANIMATION,
      TRIGGERS,
      FRAME,
      TRIGGER
    };

    class Item
    {
    public:
      Type type{NONE};
      Type ownerType{NONE};
      std::string name{};
      bool isInterpolated{false};
      bool isLoop{false};
      bool isVisible{false};
      bool isShowRect{false};
      glm::vec2 crop{};
      glm::vec2 pivot{};
      glm::vec2 position{};
      glm::vec2 scale{100, 100};
      glm::vec2 size{};
      glm::vec3 colorOffset{};
      glm::vec4 tint{1, 1, 1, 1};
      int atFrame{-1};
      int duration{1};
      int eventID{-1};
      int fps{30};
      int frameNum{1};
      int id{-1};
      int itemID{-1};
      int ownerID{-1};
      int animationID{-1};
      int spritesheetID{-1};
      int regionID{-1};
      int version{0};
      float rotation{};
      resource::Texture texture{};
      resource::Audio sound{};
      std::filesystem::path path{};
      std::string createdBy{};
      std::string createdOn{};
      std::vector<int> soundIDs{};

      Item() = default;
      Item(tinyxml2::XMLElement*, Type);
      tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&) const;
      std::string to_string() const;
    };

    std::vector<Item> items{};
    std::filesystem::path path{};
    bool isValid{false};

    Anm2() = default;
    Anm2(const std::filesystem::path&);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&) const;
    bool serialize(const std::filesystem::path&, std::string* = nullptr) const;
    std::string to_string() const;
  };
}
