#pragma once

#include "canvas.h"
#include "dialog.h"
#include "imgui_.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"
#include "strings.h"

namespace anm2ed::imgui
{
  class Taskbar
  {
    Canvas generate;
    float generateTime{};
    PopupHelper generatePopup{PopupHelper(LABEL_TASKBAR_GENERATE_ANIMATION_FROM_GRID)};
    PopupHelper overwritePopup{
        PopupHelper(LABEL_TASKBAR_OVERWRITE_FILE, imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper renderPopup{
        PopupHelper(LABEL_TASKBAR_RENDER_ANIMATION, imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper configurePopup{PopupHelper(LABEL_TASKBAR_CONFIGURE)};
    PopupHelper aboutPopup{PopupHelper(LABEL_TASKBAR_ABOUT)};
    Settings editSettings{};
    int selectedShortcut{-1};
    int creditsIndex{};
    bool isQuittingMode{};

  public:
    float height{};

    Taskbar();
    void update(Manager&, Settings&, Resources&, Dialog&, bool&);
  };
};
