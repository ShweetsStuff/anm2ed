#pragma once

#include <vector>

#include "clipboard.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "strings.hpp"

namespace anm2ed::imgui
{
  struct FrameMoveDrag
  {
    anm2::Type type{anm2::NONE};
    int itemID{-1};
    int animationIndex{-1};
    int frameIndex{-1};
    int duration{1};
    std::vector<int> indices{};
    bool isActive{};
  };

  class Timeline
  {
    bool isDragging{};
    bool isWindowHovered{};
    bool isHorizontalScroll{};
    PopupHelper propertiesPopup{PopupHelper(LABEL_TIMELINE_PROPERTIES_POPUP, POPUP_NORMAL)};
    PopupHelper bakePopup{PopupHelper(LABEL_TIMELINE_BAKE_POPUP, POPUP_SMALL_NO_HEIGHT)};
    std::string addItemName{"New Item"};
    bool addItemIsShowRect{};
    int addItemID{-1};
    int addItemSpritesheetID{-1};
    int hoveredTime{};
    anm2::Frame* draggedFrame{};
    anm2::Type draggedFrameType{};
    int draggedFrameIndex{-1};
    int draggedFrameStart{-1};
    int draggedFrameStartDuration{-1};
    bool isDraggedFrameSnapshot{};
    bool frameFocusRequested{};
    int frameFocusIndex{-1};
    FrameMoveDrag frameMoveDrag{};
    std::vector<int> frameSelectionSnapshot{};
    std::vector<int> frameSelectionLocked{};
    bool isFrameSelectionLocked{};
    anm2::Reference frameSelectionSnapshotReference{};
    glm::vec2 scroll{};
    ImDrawList* pickerLineDrawList{};
    ImGuiStyle style{};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
