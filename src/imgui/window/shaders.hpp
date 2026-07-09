#pragma once

#include "dialog.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class ShadersWindow
  {
  public:
    std::string status{};
    int dialogSpritesheetId{-1};
    void update(Manager&, Settings&, Resources&, Dialog&);
  };
}
