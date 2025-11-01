#pragma once

#include "imgui.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"
#include "taskbar.h"

namespace anm2ed::imgui
{
  class Documents
  {
    int closeDocumentIndex{-1};
    imgui::PopupHelper closePopup{imgui::PopupHelper("Close Document", imgui::POPUP_TO_CONTENT)};

  public:
    float height{};

    void update(Taskbar&, Manager&, Settings&, Resources&, bool&);
  };
}
