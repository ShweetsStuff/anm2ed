#pragma once

#include "dialog.hpp"
#include "manager.hpp"
#include "resources.hpp"

namespace anm2ed::imgui::wizard
{
  class RenderAnimation
  {

  public:
    bool isEnd{};

    void range_to_frames_set(Manager&, Document&);
    void range_to_animation_set(Manager&, Document&);
    void reset(Manager&, Document&, Settings&);
    void update(Manager&, Document&, Resources&, Settings&, Dialog&);
  };
}
