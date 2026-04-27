#pragma once

#include "dialog.hpp"
#include "manager_new.hpp"

namespace anm2ed::ui_new
{
  class Taskbar
  {
  public:
    float height{};

    void update(Dialog&, ManagerNew&, bool&);
  };
}
