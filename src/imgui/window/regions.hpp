#pragma once

#include "clipboard.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class Regions
  {
  public:
    anm2::Spritesheet::Region editRegion{};
    int newRegionId{-1};
    imgui::PopupHelper propertiesPopup{imgui::PopupHelper(LABEL_REGION_PROPERTIES, imgui::POPUP_SMALL_NO_HEIGHT)};

    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
