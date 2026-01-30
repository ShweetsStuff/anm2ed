#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <tinyxml2/tinyxml2.h>

#include "texture.h"
#include "types.h"

namespace anm2ed::anm2
{
  class Spritesheet
  {
  public:
    struct Region
    {
      std::string name{};
      glm::vec2 crop{};
      glm::vec2 pivot{};
      glm::vec2 size{};
    };

    std::filesystem::path path{};
    resource::Texture texture;

    std::map<int, Region> regions{};

    Spritesheet() = default;
    Spritesheet(tinyxml2::XMLElement*, int&);
    Spritesheet(const std::filesystem::path&, const std::filesystem::path& = {});
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, int);
    std::string to_string(int id);
    std::string region_to_string(int id);
    bool regions_deserialize(const std::string&, types::merge::Type, std::string* = nullptr);
    bool save(const std::filesystem::path&, const std::filesystem::path& = {});
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    void reload(const std::filesystem::path&);
    bool is_valid();
  };
}
