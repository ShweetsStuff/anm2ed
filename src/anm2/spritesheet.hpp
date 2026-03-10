#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <cstdint>
#include <tinyxml2/tinyxml2.h>

#include "texture.h"
#include "anm2_type.h"
#include "types.h"
#include "origin.h"

namespace anm2ed::anm2
{
  class Spritesheet
  {
  public:
    struct Region
    {
      using Origin = origin::Type;
      static constexpr Origin TOP_LEFT = origin::TOP_LEFT;
      static constexpr Origin ORIGIN_CENTER = origin::ORIGIN_CENTER;
      static constexpr Origin CUSTOM = origin::CUSTOM;

      std::string name{};
      glm::vec2 crop{};
      glm::vec2 pivot{};
      glm::vec2 size{};
      Origin origin{CUSTOM};
    };

    std::filesystem::path path{};
    resource::Texture texture;

    std::map<int, Region> regions{};
    std::vector<int> regionOrder{};

    Spritesheet() = default;
    Spritesheet(tinyxml2::XMLElement*, int&);
    Spritesheet(const std::filesystem::path&, const std::filesystem::path& = {});
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, int, Flags = 0);
    std::string to_string(int id);
    std::string region_to_string(int id);
    bool regions_deserialize(const std::string&, types::merge::Type, std::string* = nullptr);
    bool save(const std::filesystem::path&, const std::filesystem::path& = {});
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int, Flags = 0);
    void reload(const std::filesystem::path&, const std::filesystem::path& = {});
    bool is_valid();
    uint64_t hash() const;
  };
}
