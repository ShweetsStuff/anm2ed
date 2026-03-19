#pragma once

#include "animation.hpp"

namespace anm2ed::anm2
{
  constexpr auto MERGED_STRING = "(Merged)";

  struct Animations
  {
    std::string defaultAnimation{};
    std::vector<Animation> items{};

    Animations() = default;
    Animations(tinyxml2::XMLElement*);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, Flags = 0);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, Flags = 0);
    int length();
  };
}
