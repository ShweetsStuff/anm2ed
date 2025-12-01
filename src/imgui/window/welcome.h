#pragma once

#include "documents.h"
#include "manager.h"
#include "strings.h"
#include "taskbar.h"

namespace anm2ed::imgui
{
  class Welcome
  {
    PopupHelper restorePopup{PopupHelper(LABEL_WELCOME_RESTORE_POPUP, imgui::POPUP_SMALL_NO_HEIGHT)};

  public:
    void update(Manager&, Resources&, Dialog&, Taskbar&, Documents&);
  };
};
