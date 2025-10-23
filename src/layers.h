#pragma once

#include "document.h"
#include "resources.h"
#include "settings.h"

#include "imgui.h"

namespace anm2ed::layers
{
  class Layers
  {
    bool isAdd{};
    imgui::PopupHelper propertiesPopup{imgui::PopupHelper("Layer Properties", imgui::POPUP_SMALL, true)};
    imgui::MultiSelectStorage storage;
    anm2::Layer editLayer{};
    std::set<int> unusedLayerIDs{};

  public:
    void update(document::Document& document, settings::Settings& settings, resources::Resources& resources);
  };
}