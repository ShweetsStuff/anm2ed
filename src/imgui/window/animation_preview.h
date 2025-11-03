#pragma once

#include "canvas.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class AnimationPreview : public Canvas
  {
    bool isPreviewHovered{};
    bool isSizeTrySet{true};
    Settings savedSettings{};
    float savedZoom{};
    glm::vec2 savedPan{};
    glm::ivec2 mousePos{};
    std::vector<resource::Texture> renderFrames{};

  public:
    AnimationPreview();
    void tick(Manager&, Document&, Settings&);
    void update(Manager&, Settings&, Resources&);
  };
}
