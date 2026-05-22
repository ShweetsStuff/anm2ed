#pragma once

#include <set>
#include <string>

#include "document.hpp"
#include "manager.hpp"
#include "settings.hpp"
#include "strings.hpp"

namespace anm2ed::imgui::popup
{
  class ItemProperties
  {
    PopupHelper popup{PopupHelper(LABEL_TIMELINE_PROPERTIES_POPUP, POPUP_NORMAL)};
    std::string addItemName{"New Item"};
    bool addItemIsShowRect{};
    int addItemID{-1};
    int addItemSpritesheetID{-1};

    void reset();
    std::set<int> unused_items_get(Anm2&, const Element*, int);

  public:
    void open();
    bool update(Manager&, Settings&, Document&, Reference&);
  };
}
