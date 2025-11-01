#pragma once

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
    int frameTime{-1};

    auto operator<=>(const Reference&) const = default;
  };

  constexpr anm2::Reference REFERENCE_DEFAULT = {-1, anm2::NONE, -1, -1, -1};

  class Anm2
  {
  public:
    Info info{};
    Content content{};
    Animations animations{};

    Anm2();
    bool serialize(const std::string&, std::string* = nullptr);
    std::string to_string();
    Anm2(const std::string&, std::string* = nullptr);
    uint64_t hash();
    Animation* animation_get(Reference);
    std::vector<std::string> animation_names_get();

    Item* item_get(Reference);

    Frame* frame_get(Reference);

    bool spritesheet_add(const std::string&, const std::string&, int&);
    Spritesheet* spritesheet_get(int);
    void spritesheet_remove(int);
    std::set<int> spritesheets_unused();
    std::vector<std::string> spritesheet_names_get();

    int layer_add();
    Reference layer_add(Reference = REFERENCE_DEFAULT, std::string = {}, int = 0,
                        types::locale::Type = types::locale::GLOBAL);
    std::set<int> layers_unused(Reference = REFERENCE_DEFAULT);

    Reference null_add(Reference = REFERENCE_DEFAULT, std::string = {}, types::locale::Type = types::locale::GLOBAL);
    std::set<int> nulls_unused(Reference = REFERENCE_DEFAULT);

    bool sound_add(const std::string& directory, const std::string& path, int& id);
    std::vector<std::string> sound_names_get();
    std::set<int> sounds_unused();

    void event_add(int&);
    std::set<int> events_unused(Reference = REFERENCE_DEFAULT);
    std::vector<std::string> event_names_get();
    void bake(Reference, int = 1, bool = true, bool = true);
    void generate_from_grid(Reference, glm::ivec2, glm::ivec2, glm::ivec2, int, int, int);
  };
}
