#pragma once

#include <cstdint>
#include <vector>

#include "manager.h"
#include "resources.h"
#include "settings.h"
#include "localization.h"
#include "ui/taskbar.h"

namespace anm2ed::imgui
{
  class Documents
  {
    int closeDocumentIndex{-1};
    std::vector<uint64_t> pendingDocumentTabIds{};
    imgui::PopupHelper closePopup{imgui::PopupHelper(LABEL_DOCUMENT_CLOSE, imgui::POPUP_TO_CONTENT)};

  public:
    float height{};

    void pending_document_order_apply(Manager&);
    void update(Taskbar&, Manager&, Settings&, Resources&, bool&);
  };
}
