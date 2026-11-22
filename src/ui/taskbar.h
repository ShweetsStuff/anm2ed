#pragma once

#include <filesystem>

#include "canvas.h"
#include "dialog.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"
#include "localization.h"
#include "ui/popup.h"

#include "ui/about.h"
#include "ui/change_all_frame_properties.h"
#include "ui/configure.h"
#include "ui/generate_animation_from_grid.h"
#include "ui/render_animation.h"

class Taskbar
{
    struct PendingSave
    {
        int index = -1;
        std::filesystem::path path;
        bool isQueued = false;
    };

    ChangeAllFrameProperties changeAllFrameProperties = {};
    About about = {};
    Configure configure = {};
    GenerateAnimationFromGrid generateAnimationFromGrid = {};
    RenderAnimation renderAnimation = {};

    PopupHelper generatePopup = PopupHelper(LABEL_TASKBAR_GENERATE_ANIMATION_FROM_GRID);
    PopupHelper generateRegionsPopup{
        PopupHelper(LABEL_TASKBAR_GENERATE_REGIONS_FROM_ANIMATIONS, POPUP_SMALL_NO_HEIGHT)};
    PopupHelper changePopup = PopupHelper(LABEL_CHANGE_ALL_FRAME_PROPERTIES, POPUP_NORMAL_NO_HEIGHT);
    PopupHelper overwritePopup = PopupHelper(LABEL_TASKBAR_OVERWRITE_FILE, POPUP_SMALL_NO_HEIGHT);
    PopupHelper isaacPopup = PopupHelper(LABEL_ISAAC_COMPATIBILITY_WARNING, POPUP_SMALL_NO_HEIGHT);
    PendingSave isaacSave = {};
    PopupHelper vectorPopup = PopupHelper(LABEL_CONVERT_SVG_POPUP, POPUP_SMALL_NO_HEIGHT);
    PopupHelper renderPopup = PopupHelper(LABEL_TASKBAR_RENDER_ANIMATION, POPUP_SMALL_NO_HEIGHT);
    PopupHelper configurePopup = PopupHelper(LABEL_TASKBAR_CONFIGURE);
    int generateRegionsTarget = 0;
    bool isGenerateRegionsMapFrames = true;

    bool save_execute(Manager&, const PendingSave&);
    void save_enqueue(Manager&, const PendingSave&);
    bool save_request(Manager&, Settings&, int = -1, const std::filesystem::path& = {}, bool = false);

public:
    PopupHelper aboutPopup = PopupHelper(LABEL_TASKBAR_ABOUT);
    float height = 0;

    void update(Manager&, Settings&, Resources&, Dialog&, bool&);
    bool save_manual(Manager&, Settings&, int = -1, const std::filesystem::path& = {});
};
