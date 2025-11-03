#pragma once

#include "canvas.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class SpritesheetEditor : public Canvas
  {
    glm::vec2 mousePos{};
    glm::vec2 previousMousePos{};
    glm::vec2 cropAnchor{};

  public:
    SpritesheetEditor();
    void update(Manager&, Settings&, Resources&);
  };
}
