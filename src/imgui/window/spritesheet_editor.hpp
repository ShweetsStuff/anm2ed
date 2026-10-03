#pragma once

#include "canvas_view.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

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
