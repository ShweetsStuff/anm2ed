#pragma once

#include "resources.h"
#include "settings.h"

namespace anm2ed::tools
{
  class Tools
  {
    bool isOpenColorEdit{};
    ImVec2 colorEditPosition{};

  public:
    void update(settings::Settings& settings, resources::Resources& resources);
  };
}
