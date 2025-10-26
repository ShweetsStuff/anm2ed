#pragma once

#include "canvas.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::animation_preview
{
  class AnimationPreview : public canvas::Canvas
  {
    bool isPreviewHovered{};
    glm::vec2 mousePos{};

  public:
    AnimationPreview();
    void update(manager::Manager&, settings::Settings&, resources::Resources&);
  };
}
