#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  class Timeline
  {
    bool isDragging{};
    bool isWindowHovered{};
    bool isHorizontalScroll{};
    PopupHelper propertiesPopup{PopupHelper("Item Properties", POPUP_NORMAL)};
    PopupHelper bakePopup{PopupHelper("Bake", POPUP_TO_CONTENT)};
    std::string addItemName{};
    bool addItemIsRect{};
    int addItemID{-1};
    int addItemSpritesheetID{-1};
    bool isUnusedItemsSet{};
    std::set<int> unusedItems{};
    glm::vec2 scroll{};
    ImDrawList* pickerLineDrawList{};
    ImGuiStyle style{};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
