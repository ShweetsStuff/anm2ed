#pragma once

#include <filesystem>
#include <string>
#include <tinyxml2/tinyxml2.h>

#include "types.hpp"

#include "animations.hpp"
#include "content.hpp"
#include "info.hpp"

namespace anm2ed::anm2
{
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
    bool isValid{true};
    Info info{};
    Content content{};
    Animations animations{};

    Anm2();
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, Flags = 0);
    bool serialize(const std::filesystem::path&, std::string* = nullptr, Flags = 0);
    std::string to_string(Flags = 0);
    Anm2(const std::filesystem::path&, std::string* = nullptr);
    uint64_t hash();
    Anm2 normalized_for_serialize() const;

    Spritesheet* spritesheet_get(int);
    bool spritesheet_add(const std::filesystem::path&, const std::filesystem::path&, int&);
    bool spritesheet_pack(int, int);
    bool regions_trim(int, const std::set<int>&);
    std::vector<std::string> spritesheet_labels_get();
    std::vector<int> spritesheet_ids_get();
    std::set<int> spritesheets_unused();
    bool spritesheets_merge(const std::set<int>&, SpritesheetMergeOrigin, bool, bool, origin::Type);
    bool spritesheets_deserialize(const std::string&, const std::filesystem::path&, types::merge::Type type,
                                  std::string*);
    std::vector<std::string> region_labels_get(Spritesheet&);
    std::vector<int> region_ids_get(Spritesheet&);
    std::set<int> regions_unused(Spritesheet&);
    void scan_and_set_regions();

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
    std::vector<int> event_ids_get();
    std::set<int> events_unused();
    bool events_deserialize(const std::string&, types::merge::Type, std::string*);

    bool sound_add(const std::filesystem::path& directory, const std::filesystem::path& path, int& id);
    std::vector<std::string> sound_labels_get();
    std::vector<int> sound_ids_get();
    std::set<int> sounds_unused();
    bool sounds_deserialize(const std::string&, const std::filesystem::path&, types::merge::Type, std::string*);

    Animation* animation_get(int);
    std::vector<std::string> animation_labels_get();
    int animations_merge(int, std::set<int>&, types::merge::Type = types::merge::APPEND, bool = true);
    bool animations_deserialize(const std::string&, int, std::set<int>&, std::string* = nullptr);
    Frame frame_effective(int, const Frame&) const;
    glm::vec4 animation_rect(Animation&, bool) const;

    Item* item_get(int, Type, int = -1);
    Reference layer_animation_add(Reference = {}, int = -1, std::string = {}, int = 0,
                                  types::destination::Type = types::destination::ALL);
    Reference null_animation_add(Reference = {}, std::string = {}, bool = false,
                                 types::destination::Type = types::destination::ALL);

    Frame* frame_get(int, Type, int, int = -1);
    void merge(const Anm2&, const std::filesystem::path&, const std::filesystem::path&);
  };
}
