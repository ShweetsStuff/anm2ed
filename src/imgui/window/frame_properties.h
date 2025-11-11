#pragma once

#include "manager.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class FrameProperties
  {
  public:
    void update(Manager&, Settings&);
  };
}
