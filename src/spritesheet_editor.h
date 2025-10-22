#pragma once

#include "canvas.h"
#include "document_manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::spritesheet_editor
{
  class SpritesheetEditor : public canvas::Canvas
  {
    glm::vec2 mousePos{};

  public:
    SpritesheetEditor();
    void update(document_manager::DocumentManager& manager, settings::Settings& settings,
                resources::Resources& resources);
  };
}
