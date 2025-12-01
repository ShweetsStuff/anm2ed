#pragma once

#include "manager.h"
#include "resources.h"
#include "settings.h"
#include "strings.h"
#include "taskbar.h"

namespace anm2ed::imgui
{
  class Documents
  {
    int closeDocumentIndex{-1};
    imgui::PopupHelper closePopup{imgui::PopupHelper(LABEL_DOCUMENT_CLOSE, imgui::POPUP_TO_CONTENT)};

  public:
    float height{};

    void update(Taskbar&, Manager&, Settings&, Resources&, bool&);
  };
}
