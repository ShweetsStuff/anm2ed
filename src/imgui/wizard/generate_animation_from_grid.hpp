#pragma once

#include "canvas.hpp"
#include "document.hpp"
#include "resources.hpp"
#include "settings.hpp"

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
