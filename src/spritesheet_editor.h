#pragma once

#include "canvas.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::spritesheet_editor
{
  class SpritesheetEditor : public canvas::Canvas
  {
    glm::vec2 mousePos{};
    glm::vec2 previousMousePos{};

  public:
    SpritesheetEditor();
    void update(manager::Manager&, settings::Settings&, resources::Resources&);
  };
}
