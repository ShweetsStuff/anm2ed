#pragma once

#include <set>
#include <vector>

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::imgui
{
  struct FrameDragDrop
  {
    anm2::Type type{anm2::NONE};
    int itemID{-1};
    int animationIndex{-1};
    std::vector<int> selection{};
  };

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
    int hoveredTime{};
    anm2::Frame* draggedTrigger{};
    int draggedTriggerIndex{-1};
    int draggedTriggerAtFrameStart{-1};
    bool isDraggedTriggerSnapshot{};
    FrameDragDrop frameDragDrop{};
    std::vector<int> frameSelectionSnapshot{};
    std::vector<int> frameSelectionLocked{};
    bool isFrameSelectionLocked{};
    anm2::Reference frameSelectionSnapshotReference{};
    std::set<int> unusedItems{};
    glm::vec2 scroll{};
    ImDrawList* pickerLineDrawList{};
    ImGuiStyle style{};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
