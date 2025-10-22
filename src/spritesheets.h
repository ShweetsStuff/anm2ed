#pragma once

#include "dialog.h"
#include "document_manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::spritesheets
{
  class Spritesheets
  {
  public:
    void update(document_manager::DocumentManager& manager, settings::Settings& settings,
                resources::Resources& resources, dialog::Dialog& dialog);
  };
}
