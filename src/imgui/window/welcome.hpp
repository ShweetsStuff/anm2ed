#pragma once

#include "documents.hpp"
#include "taskbar.hpp"

namespace anm2ed::imgui
{
  class Welcome
  {
  public:
    void update(Manager&, Resources&, Dialog&, Taskbar&, Documents&);
  };
};
