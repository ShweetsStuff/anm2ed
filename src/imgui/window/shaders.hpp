#pragma once

#include "dialog.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "util/imgui/popup.hpp"

namespace anm2ed::imgui
{
  class ShadersWindow
  {
  public:
    std::string status{};
    int dialogShaderId{-1};
    int popupShaderId{-1};
    int newElementId{-1};
    PopupHelper propertiesPopup{PopupHelper(LABEL_SHADER_PROPERTIES, POPUP_NORMAL)};
    void update(Manager&, Settings&, Resources&, Dialog&);
  };
}
