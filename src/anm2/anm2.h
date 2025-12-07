#pragma once

#include <filesystem>
#include <string>
#include <tinyxml2/tinyxml2.h>

#include "types.h"

#include "animations.h"
#include "content.h"
#include "info.h"

namespace anm2ed::anm2
{
  constexpr auto NO_PATH = "[No Path]";

  struct Reference
  {
    int animationIndex{-1};
    Type itemType{NONE};
    int itemID{-1};
    int frameIndex{-1};

    auto operator<=>(const Reference&) const = default;
  };

  class Anm2
  {
  public:
    Info info{};
    Content content{};
    Animations animations{};

    Anm2();
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&);
    bool serialize(const std::string&, std::string* = nullptr);
    std::string to_string();
    Anm2(const std::string&, std::string* = nullptr);
    uint64_t hash();

    Spritesheet* spritesheet_get(int);
    bool spritesheet_add(const std::string&, const std::string&, int&);
    std::vector<std::string> spritesheet_labels_get();
    std::set<int> spritesheets_unused();
    bool spritesheets_deserialize(const std::string&, const std::string&, types::merge::Type type, std::string*);

    void layer_add(int&);
    std::set<int> layers_unused();
    std::set<int> layers_unused(Animation&);
    bool layers_deserialize(const std::string&, types::merge::Type, std::string*);

    void null_add(int&);
    std::set<int> nulls_unused();
    std::set<int> nulls_unused(Animation&);
    bool nulls_deserialize(const std::string&, types::merge::Type, std::string*);

    void event_add(int&);
    std::vector<std::string> event_labels_get();
    std::set<int> events_unused();
    bool events_deserialize(const std::string&, types::merge::Type, std::string*);

    bool sound_add(const std::string& directory, const std::string& path, int& id);
    std::vector<std::string> sound_labels_get();
    std::set<int> sounds_unused();
    bool sounds_deserialize(const std::string&, const std::string&, types::merge::Type, std::string*);

    Animation* animation_get(int);
    std::vector<std::string> animation_labels_get();
    int animations_merge(int, std::set<int>&, types::merge::Type = types::merge::APPEND, bool = true);
    bool animations_deserialize(const std::string&, int, std::set<int>&, std::string* = nullptr);

    Item* item_get(int, Type, int = -1);
    Reference layer_animation_add(Reference = {}, std::string = {}, int = 0,
                                  types::destination::Type = types::destination::ALL);
    Reference null_animation_add(Reference = {}, std::string = {}, types::destination::Type = types::destination::ALL);

    Frame* frame_get(int, Type, int, int = -1);
    void merge(const Anm2& source, const std::filesystem::path& destinationDirectory = {},
               const std::filesystem::path& sourceDirectory = {});
  };
}
