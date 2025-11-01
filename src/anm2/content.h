#pragma once

#include <map>
#include <set>

#include "event.h"
#include "layer.h"
#include "null.h"
#include "sound.h"
#include "spritesheet.h"

#include "types.h"

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

    bool spritesheet_add(const std::string&, const std::string&, int&);
    std::set<int> spritesheets_unused();
    void spritesheet_remove(int&);
    bool spritesheets_deserialize(const std::string&, const std::string&, types::merge::Type, std::string* = nullptr);

    void layer_add(int&);
    bool layers_deserialize(const std::string&, types::merge::Type, std::string* = nullptr);

    void null_add(int&);
    bool nulls_deserialize(const std::string&, types::merge::Type, std::string* = nullptr);

    void event_add(int&);
    bool events_deserialize(const std::string&, types::merge::Type, std::string* = nullptr);

    void sound_add(int&);
    bool sounds_deserialize(const std::string&, const std::string&, types::merge::Type, std::string* = nullptr);
  };
}