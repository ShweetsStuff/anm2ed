#pragma once

#include <filesystem>
#include <string>
#include <tinyxml2/tinyxml2.h>

#include "texture.h"

namespace anm2ed::anm2
{
  class Spritesheet
  {
  public:
    std::filesystem::path path{};
    resource::Texture texture;

    Spritesheet() = default;
    Spritesheet(tinyxml2::XMLElement*, int&);
    Spritesheet(const std::string&, const std::string& = {});
    Spritesheet(const std::filesystem::path&, const std::filesystem::path& = {});
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, int);
    std::string to_string(int id);
    bool save(const std::string&, const std::string& = {});
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    void reload(const std::filesystem::path&);
    bool is_valid();
  };
}
