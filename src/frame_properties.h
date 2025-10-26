#pragma once

#include "manager.h"
#include "settings.h"

namespace anm2ed::frame_properties
{
  class FrameProperties
  {
  public:
    void update(manager::Manager&, settings::Settings&);
  };
}
