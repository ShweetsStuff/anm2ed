#pragma once

#include "manager.hpp"
#include "strings.hpp"

namespace anm2ed::imgui
{
  class AutosaveRestore
  {
    PopupHelper popup{PopupHelper(LABEL_WELCOME_RESTORE_POPUP, imgui::POPUP_SMALL_NO_HEIGHT)};

  public:
    void update(Manager&);
  };
}
