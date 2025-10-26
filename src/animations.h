#pragma once

#include "clipboard.h"
#include "imgui.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::animations
{
  class Animations
  {
    imgui::PopupHelper mergePopup{imgui::PopupHelper("Merge Animations")};

  public:
    void update(manager::Manager&, settings::Settings&, resources::Resources&, clipboard::Clipboard&);
  };
}
