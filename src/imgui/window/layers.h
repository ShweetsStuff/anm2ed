#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class Layers
  {
  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
