#pragma once

#include <filesystem>
#include <tinyxml2/tinyxml2.h>

#include "audio.h"

namespace anm2ed::anm2
{
  class Sound
  {
  public:
    std::filesystem::path path{};
    resource::Audio audio{};

    Sound() = default;
    Sound(Sound&&) noexcept = default;
    Sound& operator=(Sound&&) noexcept = default;

    Sound(const Sound&);
    Sound& operator=(const Sound&);
    Sound(tinyxml2::XMLElement*, int&);
    Sound(const std::string&, const std::string&);
    Sound(const std::filesystem::path&, const std::filesystem::path&);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, int);
    std::string to_string(int);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    void reload(const std::filesystem::path&);
    bool is_valid();
    void play();
  };
}
