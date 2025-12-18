#pragma once

#include "canvas.h"
#include "document.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui::wizard
{
  class GenerateAnimationFromGrid : public Canvas
  {
    float time{};

  public:
    bool isEnd{};

    GenerateAnimationFromGrid();
    void update(Document&, Resources&, Settings&);
  };
}
