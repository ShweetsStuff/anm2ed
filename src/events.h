#pragma once

#include "document_manager.h"
#include "imgui.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::events
{
  class Events
  {
    imgui::MultiSelectStorage storage{};
    std::set<int> unusedEventIDs{};

  public:
    void update(document_manager::DocumentManager& manager, settings::Settings& settings,
                resources::Resources& resources);
  };
}
