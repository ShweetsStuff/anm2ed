#pragma once

#include <filesystem>
#include <tinyxml2/tinyxml2.h>

#include "audio.h"

namespace anm2ed::anm2
{
  constexpr auto SOUND_FORMAT = "#{} {}";
  constexpr auto SOUND_FORMAT_C = "#%d %s";

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
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, int);
    std::string to_string(int);
  };
}