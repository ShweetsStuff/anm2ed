#pragma once

#include "canvas.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class SpritesheetEditor : public Canvas
  {
    glm::vec2 mousePos{};
    glm::vec2 previousMousePos{};
    glm::vec2 cropAnchor{};
    glm::vec2 checkerPan{};
    glm::vec2 checkerSyncPan{};
    float checkerSyncZoom{};
    bool isCheckerPanInitialized{};
    bool hasPendingZoomPanAdjust{};
    bool isFocused{};
    int hoveredRegionId{-1};

  public:
    SpritesheetEditor();
    bool is_focused_get() const;
    void update(Manager&, Settings&, Resources&);
  };
}
