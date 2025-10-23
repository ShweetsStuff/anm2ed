#pragma once

#include "document_manager.h"
#include "imgui.h"
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

    void update(taskbar::Taskbar& taskbar, document_manager::DocumentManager& manager, resources::Resources& resources);
  };
}
