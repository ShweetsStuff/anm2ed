#pragma once

#include <set>

#include "anm2/anm2.hpp"
#include "util/imgui/multiselect.hpp"

namespace anm2ed
{
  class Storage
  {
  public:
    int reference{-1};
    int hovered{-1};
    std::vector<std::string> labelsString{};
    std::vector<const char*> labels{};
    std::vector<int> ids{};
    imgui::MultiSelectStorage selection{};
    std::set<Reference> references{};

    void clear();
    void labels_set(std::vector<std::string>);
    void labels_set(std::vector<std::string>, std::vector<int>);
  };
}
