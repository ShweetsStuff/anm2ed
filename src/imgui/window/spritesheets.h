#pragma once

#include "clipboard.h"
#include "dialog.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class Spritesheets
  {
    int newSpritesheetId{-1};
    PopupHelper mergePopup{PopupHelper(LABEL_SPRITESHEETS_MERGE_POPUP, imgui::POPUP_SMALL_NO_HEIGHT)};
    std::set<int> mergeSelection{};

  public:
    void update(Manager&, Settings&, Resources&, Dialog&, Clipboard& clipboard);
  };
}
