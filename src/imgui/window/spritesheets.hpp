#pragma once

#include "clipboard.hpp"
#include "dialog.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class Spritesheets
  {
    int newSpritesheetId{-1};
    PopupHelper mergePopup{PopupHelper(LABEL_SPRITESHEETS_MERGE_POPUP, imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper packPopup{PopupHelper(LABEL_SPRITESHEETS_PACK_POPUP, imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper overwritePopup{PopupHelper(LABEL_TASKBAR_OVERWRITE_FILE, imgui::POPUP_SMALL_NO_HEIGHT)};
    std::set<int> mergeSelection{};
    int packId{-1};
    std::set<int> saveSelection{};

  public:
    void update(Manager&, Settings&, Resources&, Dialog&, Clipboard& clipboard);
  };
}
