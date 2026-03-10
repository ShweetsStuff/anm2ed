#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

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
