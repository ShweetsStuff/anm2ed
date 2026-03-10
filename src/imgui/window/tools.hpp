#pragma once

#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "strings.hpp"

namespace anm2ed::imgui
{
  class Tools
  {
    bool isOpenColorEdit{};
    ImVec2 colorEditPosition{};

    PopupHelper colorEditPopup{
        PopupHelper(LABEL_TOOLS_COLOR_EDIT_POPUP, POPUP_TO_CONTENT, POPUP_BY_ITEM)};

  public:
    void update(Manager&, Settings&, Resources&);
  };
}
