#pragma once

#include "anm2/anm2.h"
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

    void labels_set(std::vector<std::string>);
  };

  class FrameStorage
  {
  public:
    anm2::Type referenceType{anm2::NONE};
    int referenceID{-1};
    int referenceFrameIndex{-1};
  };
}