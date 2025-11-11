#pragma once

#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class Tools
  {
    bool isOpenColorEdit{};
    ImVec2 colorEditPosition{};

    PopupHelper colorEditPopup{PopupHelper("##Color Edit", POPUP_TO_CONTENT, POPUP_BY_ITEM)};

  public:
    void update(Manager&, Settings&, Resources&);
  };
}
