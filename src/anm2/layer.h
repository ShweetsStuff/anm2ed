#pragma once

#include <string>
#include <tinyxml2/tinyxml2.h>

namespace anm2ed::anm2
{
  constexpr auto LAYER_FORMAT = "#{} {} (Spritesheet: #{})";
  constexpr auto LAYER_NO_SPRITESHEET_FORMAT = "#{} {} (No Spritesheet)";

  class Layer
  {
  public:
    std::string name{"New Layer"};
    int spritesheetID{};

    Layer() = default;
    Layer(tinyxml2::XMLElement*, int&);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, int);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    std::string to_string(int);
    bool is_spritesheet_valid();
  };
}