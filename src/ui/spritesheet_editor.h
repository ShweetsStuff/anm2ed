#pragma once

#include "ui/canvas_view.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class SpritesheetEditor : public CanvasView
  {
    glm::vec2 previousMousePos{};
    glm::vec2 cropAnchor{};
    int hoveredRegionId{-1};

  public:
    void update(Manager&, Settings&, Resources&);
  };
}
