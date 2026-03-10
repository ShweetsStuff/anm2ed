#pragma once

#include <glm/vec2.hpp>

#include "manager.h"
#include "wizard/change_all_frame_properties.h"

namespace anm2ed::imgui
{
  class FrameProperties
  {
    wizard::ChangeAllFrameProperties changeAllFrameProperties{};

  public:
    void update(Manager&, Settings&);
  };
}
