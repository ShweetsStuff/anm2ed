#pragma once

#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::tools
{
  class Tools
  {
    bool isOpenColorEdit{};
    ImVec2 colorEditPosition{};

  public:
    void update(manager::Manager&, settings::Settings&, resources::Resources&);
  };
}
