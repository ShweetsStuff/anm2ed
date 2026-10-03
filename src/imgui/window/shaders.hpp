#pragma once

#include "panel.hpp"

namespace anm2ed::imgui
{
  struct ShadersPanel
  {
    PanelState state{};
    std::string status{};
    int dialogShaderId{-1};
    int popupShaderId{-1};
    PopupHelper propertiesPopup{PopupHelper(LABEL_SHADER_PROPERTIES, POPUP_NORMAL)};
  };

  void shaders_update(Panel&, ShadersPanel&);
}
