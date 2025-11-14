#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class Nulls
  {
    int newNullId{-1};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
