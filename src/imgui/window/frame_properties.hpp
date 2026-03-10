#pragma once

#include <glm/vec2.hpp>

#include "manager.hpp"
#include "wizard/change_all_frame_properties.hpp"

namespace anm2ed::imgui
{
  class FrameProperties
  {
    wizard::ChangeAllFrameProperties changeAllFrameProperties{};

  public:
    void update(Manager&, Settings&);
  };
}
