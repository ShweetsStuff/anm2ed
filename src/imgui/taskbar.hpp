#pragma once

#include <filesystem>

#include "canvas.hpp"
#include "dialog.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "strings.hpp"
#include "util/imgui/popup.hpp"

#include "wizard/about.hpp"
#include "wizard/change_all_frame_properties.hpp"
#include "wizard/configure.hpp"
#include "wizard/generate_animation_from_grid.hpp"
#include "wizard/render_animation.hpp"

namespace anm2ed::imgui
{
  class Taskbar
  {
    struct PendingSave
    {
      int index{-1};
      std::filesystem::path path{};
      bool isQueued{};
    };

    wizard::ChangeAllFrameProperties changeAllFrameProperties{};
    wizard::About about{};
    wizard::Configure configure{};
    wizard::GenerateAnimationFromGrid generateAnimationFromGrid{};
    wizard::RenderAnimation renderAnimation{};

    Canvas generate;
    PopupHelper generatePopup{PopupHelper(LABEL_TASKBAR_GENERATE_ANIMATION_FROM_GRID)};
    PopupHelper generateRegionsPopup{
        PopupHelper(LABEL_TASKBAR_GENERATE_REGIONS_FROM_ANIMATIONS, imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper changePopup{PopupHelper(LABEL_CHANGE_ALL_FRAME_PROPERTIES, imgui::POPUP_NORMAL_NO_HEIGHT)};
    PopupHelper overwritePopup{PopupHelper(LABEL_TASKBAR_OVERWRITE_FILE, imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper renderPopup{PopupHelper(LABEL_TASKBAR_RENDER_ANIMATION, imgui::POPUP_SMALL_NO_HEIGHT)};
    PopupHelper configurePopup{PopupHelper(LABEL_TASKBAR_CONFIGURE)};
    PopupHelper aboutPopup{PopupHelper(LABEL_TASKBAR_ABOUT)};
    Settings editSettings{};
    int generateRegionsTarget{};
    bool isGenerateRegionsMapFrames{true};
    bool isQuittingMode{};

    bool save_execute(Manager&, const PendingSave&);
    void save_enqueue(Manager&, const PendingSave&);
    bool save_request(Manager&, Settings&, int = -1, const std::filesystem::path& = {}, bool = false);

  public:
    float height{};

    void update(Manager&, Settings&, Resources&, Dialog&, bool&);
    bool save_manual(Manager&, Settings&, int = -1, const std::filesystem::path& = {});
  };
};
