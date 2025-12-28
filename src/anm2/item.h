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
    void frames_change(FrameChange, anm2::Type, ChangeType, std::set<int>&);
    bool frames_deserialize(const std::string&, Type, int, std::set<int>&, std::string*);
    void frames_bake(int, int, bool, bool);
    void frames_generate_from_grid(glm::ivec2, glm::ivec2, glm::ivec2, int, int, int);
    void frames_sort_by_at_frame();
    int frame_index_from_at_frame_get(int);
    int frame_index_from_time_get(float);
    float frame_time_from_index_get(int);
  };
}
