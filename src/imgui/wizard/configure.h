#pragma once

#include "manager.h"

namespace anm2ed::imgui::wizard
{
  class Configure
  {
    Settings temporary{};
    int selectedShortcut{-1};

  public:
    bool isSet{};

    void reset(Settings&);
    void update(Manager&, Settings&);
  };
}