#pragma once

#include "clipboard.h"
#include "dialog.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class Spritesheets
  {
    int newSpritesheetId{-1};

  public:
    void update(Manager&, Settings&, Resources&, Dialog&, Clipboard& clipboard);
  };
}
