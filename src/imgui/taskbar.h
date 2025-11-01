#pragma once

#include "canvas.h"
#include "dialog.h"
#include "imgui_.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class Taskbar
  {
    Canvas generate;
    float generateTime{};
    PopupHelper generatePopup{PopupHelper("Generate Animation from Grid")};
    PopupHelper changePopup{PopupHelper("Change All Frame Properties", imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper renderPopup{PopupHelper("Render Animation", imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper configurePopup{PopupHelper("Configure")};
    PopupHelper aboutPopup{PopupHelper("About")};
    Settings editSettings{};
    int selectedShortcut{-1};
    bool isQuittingMode{};

  public:
    float height{};

    Taskbar();
    void update(Manager&, Settings&, Resources&, Dialog&, bool&);
  };
};
