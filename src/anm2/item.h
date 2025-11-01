#pragma once

#include <set>
#include <vector>

#include "frame.h"

namespace anm2ed::anm2
{
  class Item
  {
  public:
    std::vector<Frame> frames{};
    bool isVisible{true};

    Item() = default;
    Item(tinyxml2::XMLElement*, Type, int* = nullptr);
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, Type, int);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, Type, int = -1);
    std::string to_string(Type, int = -1);
    int length(Type);
    Frame frame_generate(float, Type);
    void frames_change(anm2::FrameChange&, ChangeType, int, int = 0);
    bool frames_deserialize(const std::string&, Type, int, std::set<int>&, std::string*);
  };
}