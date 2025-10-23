#pragma once

#include "dialog.h"
#include "document_manager.h"
#include "imgui.h"
#include "settings.h"

namespace anm2ed::taskbar
{
  class Taskbar
  {
    imgui::PopupHelper configurePopup{imgui::PopupHelper("Configure")};
    imgui::PopupHelper aboutPopup{imgui::PopupHelper("About")};
    settings::Settings editSettings{};
    int selectedShortcut{};

  public:
    float height{};

    void update(settings::Settings& settings, dialog::Dialog& dialog, document_manager::DocumentManager& manager,
                bool& isQuit);
  };
};
