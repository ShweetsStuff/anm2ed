#pragma once

#include <map>
#include <string>

#include "item.h"

namespace anm2ed::anm2
{
  constexpr auto FRAME_NUM_MIN = 1;
  constexpr auto FRAME_NUM_MAX = FRAME_DURATION_MAX;

  class Animation
  {
  public:
    std::string name{};
    int frameNum{FRAME_NUM_MIN};
    bool isLoop{true};
    Item rootAnimation;
    std::unordered_map<int, Item> layerAnimations{};
    std::vector<int> layerOrder{};
    std::map<int, Item> nullAnimations{};
    Item triggers;

    Animation() = default;
    Animation(tinyxml2::XMLElement*);
    Item* item_get(Type, int = -1);
    void item_remove(Type, int = -1);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*);
    std::string to_string();
    int length();
    void fit_length();
    glm::vec4 rect(bool);
  };

}