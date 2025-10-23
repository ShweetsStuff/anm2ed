#pragma once

#include "document.h"
#include "imgui.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::animations
{
  class Animations
  {
    imgui::PopupHelper mergePopup{imgui::PopupHelper("Merge Animations")};
    imgui::MultiSelectStorage mergeStorage{};
    imgui::MultiSelectStorage storage{};
    std::set<int> mergeSelection{};
    int mergeTarget{};

  public:
    void update(document::Document& document, int& documentIndex, settings::Settings& settings,
                resources::Resources& resources);
  };
}