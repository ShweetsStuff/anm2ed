#pragma once

#include "manager_new.hpp"
#include "taskbar.hpp"

namespace anm2ed::ui_new
{
  class Documents
  {
    int syncedSelected{-1};

  public:
    float height{};

    void update(Taskbar&, ManagerNew&);
  };
}
