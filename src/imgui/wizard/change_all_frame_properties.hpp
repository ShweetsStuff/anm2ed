#pragma once

#include "document.hpp"
#include "settings.hpp"

namespace anm2ed::imgui::wizard
{
  class ChangeAllFrameProperties
  {
  public:
    bool isChanged{};

    void update(Document&, Settings&);
  };
}
