#pragma once

#include "documents.hpp"
#include "manager.hpp"
#include "strings.hpp"
#include "taskbar.hpp"

namespace anm2ed::imgui
{
  class Welcome
  {
    PopupHelper restorePopup{PopupHelper(LABEL_WELCOME_RESTORE_POPUP, imgui::POPUP_SMALL_NO_HEIGHT)};

  public:
    void update(Manager&, Resources&, Dialog&, Taskbar&, Documents&);
  };
};
