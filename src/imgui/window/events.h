#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class Events
  {
    anm2::Event editEvent{};
    PopupHelper propertiesPopup{PopupHelper("Event Properties", POPUP_SMALL_NO_HEIGHT)};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
