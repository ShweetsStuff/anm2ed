#pragma once

#include "imgui_.h"

namespace anm2ed
{
  class Storage
  {
  public:
    int reference{-1};
    int hovered{-1};
    std::set<int> unused{};
    std::vector<std::string> labelsString{};
    std::vector<const char*> labels{};
    imgui::MultiSelectStorage selection{};

    void clear();
    void labels_set(std::vector<std::string>);
  };
}