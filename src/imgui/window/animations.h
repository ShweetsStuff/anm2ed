#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class Animations
  {
    PopupHelper mergePopup{PopupHelper("Merge Animations")};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
