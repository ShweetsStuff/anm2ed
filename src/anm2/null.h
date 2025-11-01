#pragma once

#include <string>
#include <tinyxml2/tinyxml2.h>

namespace anm2ed::anm2
{
  constexpr auto NULL_FORMAT = "#{} {}";

  class Null
  {
  public:
    std::string name{"New Null"};
    bool isShowRect{};

    Null() = default;
    Null(tinyxml2::XMLElement*, int&);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument& document, int id);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    std::string to_string(int);
  };
}