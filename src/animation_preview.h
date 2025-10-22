#pragma once

#include "canvas.h"
#include "document_manager.h"
#include "playback.h"
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
    void update(document_manager::DocumentManager& manager, settings::Settings& settings,
                resources::Resources& resources, playback::Playback& playback);
  };
}