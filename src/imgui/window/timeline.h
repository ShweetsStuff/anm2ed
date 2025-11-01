#pragma once

#include "clipboard.h"
#include "document.h"
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
    int addItemSpritesheetID{};
    bool addItemIsRect{};
    int addItemID{-1};
    bool isUnusedItemsSet{};
    std::set<int> unusedItems{};
    glm::vec2 scroll{};
    ImDrawList* pickerLineDrawList{};
    ImGuiStyle style{};

    void context_menu(Document&, Settings&, Clipboard&);
    void item_child(Manager&, Document&, anm2::Animation*, Settings&, Resources&, Clipboard&, anm2::Type, int, int&);
    void items_child(Manager&, Document&, anm2::Animation*, Settings&, Resources&, Clipboard&);
    void frame_child(Document&, anm2::Animation*, Settings&, Resources&, Clipboard&, anm2::Type, int, int&, float);
    void frames_child(Document&, anm2::Animation*, Settings&, Resources&, Clipboard&);

    void popups(Document&, anm2::Animation*, Settings&);

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
