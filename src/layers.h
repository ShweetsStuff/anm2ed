#pragma once

#include "document_manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::layers
{
  class Layers
  {
    ImGuiSelectionExternalStorage storage{};

  public:
    void update(document_manager::DocumentManager& manager, settings::Settings& settings,
                resources::Resources& resources);
  };
}