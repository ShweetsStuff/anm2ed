#pragma once

#include <string>
#include <tinyxml2/tinyxml2.h>

namespace anm2ed::anm2
{
  constexpr auto FPS_MIN = 1;
  constexpr auto FPS_MAX = 120;

  class Info
  {
  public:
    std::string createdBy{"robot"};
    std::string createdOn{};
    int fps = 30;
    int version{};

    Info() = default;
    Info(tinyxml2::XMLElement*);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument& document);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*);
    std::string to_string();
  };
}