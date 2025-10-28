#pragma once

#include "canvas.h"
#include "dialog.h"
#include "imgui.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::taskbar
{
  class Taskbar
  {
    canvas::Canvas generate;
    float generateTime{};
    imgui::PopupHelper generatePopup{imgui::PopupHelper("Generate Animation from Grid")};
    imgui::PopupHelper changePopup{imgui::PopupHelper("Change All Frame Properties", imgui::POPUP_SMALL, true)};
    imgui::PopupHelper renderPopup{imgui::PopupHelper("Render Animation")};
    imgui::PopupHelper configurePopup{imgui::PopupHelper("Configure")};
    imgui::PopupHelper aboutPopup{imgui::PopupHelper("About")};
    settings::Settings editSettings{};
    int selectedShortcut{-1};
    bool isQuittingMode{};

  public:
    float height{};

    Taskbar();
    void update(manager::Manager&, settings::Settings&, resources::Resources&, dialog::Dialog&, bool&);
  };
};
