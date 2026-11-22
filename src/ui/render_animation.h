#pragma once

#include "dialog.h"
#include "manager.h"
#include "resources.h"

  class RenderAnimation
  {

  public:
    bool isEnd{};

    void range_to_frames_set(Manager&, Document&);
    void range_to_animation_set(Manager&, Document&);
    void reset(Manager&, Document&, Settings&);
    void update(Manager&, Document&, Resources&, Settings&, Dialog&);
  };
