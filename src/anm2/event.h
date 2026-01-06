#pragma once

#include <string>
#include <tinyxml2/tinyxml2.h>

namespace anm2ed::anm2
{
  class Event
  {
  public:
    std::string name{};

    Event() = default;
    Event(tinyxml2::XMLElement*, int&);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, int);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    std::string to_string(int);
  };
}