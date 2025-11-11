#pragma once

#include <map>

#include "anm2_type.h"
#include "event.h"
#include "layer.h"
#include "null.h"
#include "sound.h"
#include "spritesheet.h"

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

    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*);
    Content(tinyxml2::XMLElement*);
  };
}