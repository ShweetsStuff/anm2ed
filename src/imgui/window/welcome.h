#pragma once

#include "documents.h"
#include "manager.h"
#include "taskbar.h"

namespace anm2ed::imgui
{
  class Welcome
  {
    PopupHelper restorePopup{PopupHelper("Restore", imgui::POPUP_SMALL_NO_HEIGHT)};

  public:
    void update(Manager&, Resources&, Dialog&, Taskbar&, Documents&);
  };
};