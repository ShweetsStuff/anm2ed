#pragma once

#include "dialog.h"
#include "document.h"
#include "imgui.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::spritesheets
{
  class Spritesheets
  {
    imgui::MultiSelectStorage storage{};
    std::set<int> unusedSpritesheetIDs{};

  public:
    void update(document::Document& document, settings::Settings& settings, resources::Resources& resources,
                dialog::Dialog& dialog);
  };
}
