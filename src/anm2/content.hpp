#pragma once

#include <map>

#include "event.hpp"
#include "layer.hpp"
#include "null.hpp"
#include "sound.hpp"
#include "spritesheet.hpp"

namespace anm2ed::anm2
{
  struct Content
  {
    std::map<int, Spritesheet> spritesheets{};
    std::map<int, Layer> layers{};
    std::map<int, Null> nulls{};
    std::map<int, Event> events{};
    std::map<int, Sound> sounds{};

    Content() = default;

    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, Flags = 0);
    Content(tinyxml2::XMLElement*);
  };
}
