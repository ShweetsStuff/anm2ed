#pragma once

#include "document.hpp"
#include "manager.hpp"
#include "settings.hpp"

namespace anm2ed::imgui::wizard
{
  class ChangeAllFrameProperties
  {
  public:
    bool isChanged{};

    void update(Manager&, Document&, Settings&, bool = false);
  };
}
