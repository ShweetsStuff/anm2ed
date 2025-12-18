#pragma once

#include "canvas.h"
#include "dialog.h"
#include "imgui_.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"
#include "strings.h"

#include "wizard/about.h"
#include "wizard/change_all_frame_properties.h"
#include "wizard/configure.h"
#include "wizard/generate_animation_from_grid.h"
#include "wizard/render_animation.h"

namespace anm2ed::imgui
{
  class Taskbar
  {
    wizard::ChangeAllFrameProperties changeAllFrameProperties{};
    wizard::About about{};
    wizard::Configure configure{};
    wizard::GenerateAnimationFromGrid generateAnimationFromGrid{};
    wizard::RenderAnimation renderAnimation{};

    Canvas generate;
    PopupHelper generatePopup{PopupHelper(LABEL_TASKBAR_GENERATE_ANIMATION_FROM_GRID)};
    PopupHelper changePopup{PopupHelper(LABEL_CHANGE_ALL_FRAME_PROPERTIES, imgui::POPUP_NORMAL_NO_HEIGHT)};
    PopupHelper overwritePopup{PopupHelper(LABEL_TASKBAR_OVERWRITE_FILE, imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper renderPopup{PopupHelper(LABEL_TASKBAR_RENDER_ANIMATION, imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper configurePopup{PopupHelper(LABEL_TASKBAR_CONFIGURE)};
    PopupHelper aboutPopup{PopupHelper(LABEL_TASKBAR_ABOUT)};
    Settings editSettings{};
    bool isQuittingMode{};

  public:
    float height{};

    void update(Manager&, Settings&, Resources&, Dialog&, bool&);
  };
};
