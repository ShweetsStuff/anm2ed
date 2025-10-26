#pragma once

#include "imgui.h"
#include "manager.h"
#include "resources.h"
#include "taskbar.h"

namespace anm2ed::documents
{
  class Documents
  {
    bool isCloseDocument{};
    bool isOpenCloseDocumentPopup{};
    int closeDocumentIndex{};
    imgui::PopupHelper closePopup{imgui::PopupHelper("Close Document", imgui::POPUP_TO_CONTENT)};

  public:
    float height{};

    void update(taskbar::Taskbar&, manager::Manager&, resources::Resources&);
  };
}
