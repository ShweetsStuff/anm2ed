#pragma once

#include "document.h"
#include "settings.h"

namespace anm2ed::imgui::wizard
{
  class ChangeAllFrameProperties
  {
  public:
    bool isChanged{};

    void update(Document&, Settings&);
  };
}
