#pragma once

#include <glm/vec2.hpp>

#include "manager.h"
#include "settings.h"

namespace anm2
{
  struct Frame;
}

namespace anm2ed::imgui
{
  class FrameProperties
  {
  public:
    void update(Manager&, Settings&);

  private:
    glm::vec2 cropEditingValue{};
    const anm2::Frame* cropEditingFrame{};
    bool isCropEditing{};
  };
}
