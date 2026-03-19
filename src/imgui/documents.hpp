#pragma once

#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "strings.hpp"
#include "taskbar.hpp"

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
