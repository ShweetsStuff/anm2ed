#pragma once

#include <string>

#include "document.hpp"
#include "manager.hpp"
#include "settings.hpp"
#include "strings.hpp"

namespace anm2ed::imgui::popup
{
  class ItemProperties
  {
    PopupHelper popup{PopupHelper(LABEL_TIMELINE_PROPERTIES_POPUP, POPUP_SMALL_NO_HEIGHT)};
    Manager::ItemEdit edit{};

    void reset();

  public:
    void open();
    bool update(Manager&, Settings&, Document&, Reference&);
  };
}
