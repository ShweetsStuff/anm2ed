#pragma once

#include "clipboard.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class Nulls
  {
    int newNullId{-1};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
