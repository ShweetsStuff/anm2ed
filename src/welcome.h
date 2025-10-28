#pragma once

#include "documents.h"
#include "manager.h"
#include "taskbar.h"

namespace anm2ed::welcome
{
  class Welcome
  {
    imgui::PopupHelper restorePopup{imgui::PopupHelper("Restore", imgui::POPUP_SMALL, true)};

  public:
    void update(manager::Manager&, resources::Resources&, dialog::Dialog&, taskbar::Taskbar&, documents::Documents&);
  };
};