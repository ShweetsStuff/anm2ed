#pragma once

#include "dialog.h"
#include "document_manager.h"
#include "settings.h"

namespace anm2ed::taskbar
{
  class Taskbar
  {
    bool isOpenConfigurePopup{};
    bool isOpenAboutPopup{};
    int selectedShortcut{-1};
    settings::Settings editSettings{};

  public:
    float height{};

    void update(settings::Settings& settings, dialog::Dialog& dialog, document_manager::DocumentManager& manager,
                bool& isQuit);
  };
};
