#pragma once

#include "dialog.h"
#include "imgui.h"
#include "manager.h"
#include "settings.h"

namespace anm2ed::taskbar
{
  class Taskbar
  {
    imgui::PopupHelper configurePopup{imgui::PopupHelper("Configure")};
    imgui::PopupHelper aboutPopup{imgui::PopupHelper("About")};
    settings::Settings editSettings{};
    int selectedShortcut{-1};

  public:
    float height{};

    void update(manager::Manager&, settings::Settings&, dialog::Dialog&, bool&);
  };
};
