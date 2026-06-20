#pragma once

#include <set>
#include <string>
#include <vector>

#include "clipboard.hpp"
#include "manager.hpp"
#include "popup/item_properties.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "strings.hpp"
#include "util/imgui/popup.hpp"

namespace anm2ed::imgui
{
  struct TimelineRowReference
  {
    int documentIndex{-1};
    int animationIndex{-1};
    int type{NONE};
    int id{-1};
    int index{-1};
    bool isGroup{};

    auto operator<=>(const TimelineRowReference&) const = default;
  };

  struct FrameMoveDrag
  {
    int type{NONE};
    int itemID{-1};
    int animationIndex{-1};
    int frameIndex{-1};
    int duration{1};
    std::vector<int> indices{};
    std::vector<Reference> references{};
    bool isActive{};
  };

  struct FrameDurationDrag
  {
    Reference reference{};
    int duration{1};
  };

  enum class BakeIntoOtherFramesTarget
  {
    CURRENT_SELECTION,
    ALL
  };

  class Timeline
  {
    bool isDragging{};
    bool isWindowHovered{};
    bool isHorizontalScroll{};
    popup::ItemProperties itemProperties{};
    PopupHelper bakePopup{PopupHelper(LABEL_TIMELINE_BAKE_POPUP, POPUP_SMALL_NO_HEIGHT)};
    PopupHelper bakeIntoOtherFramesPopup{PopupHelper(LABEL_BAKE_INTO_OTHER_FRAMES, POPUP_SMALL_NO_HEIGHT)};
    BakeIntoOtherFramesTarget bakeIntoOtherFramesTarget{BakeIntoOtherFramesTarget::CURRENT_SELECTION};
    bool isBakeIntoOtherFramesLayers{true};
    bool isBakeIntoOtherFramesNulls{true};
    int hoveredTime{};
    bool isFrameBoxPending{};
    bool isFrameBoxSelecting{};
    bool isFrameBoxAdditive{};
    ImVec2 frameBoxStart{};
    ImVec2 frameBoxEnd{};
    std::set<Reference> frameBoxSelection{};
    TimelineRowReference rowSelectionAnchor{};
    bool isRowSelectionAnchorSet{};
    PopupHelper groupPropertiesPopup{PopupHelper(LABEL_GROUP_PROPERTIES, POPUP_SMALL_NO_HEIGHT)};
    std::string groupName{};
    int groupAnimationIndex{-1};
    int groupType{NONE};
    int groupId{-1};
    Reference draggedFrameReference{};
    bool isDraggedFrameActive{};
    int draggedFrameType{};
    int draggedFrameIndex{-1};
    int draggedFrameStart{-1};
    int draggedFrameStartDuration{-1};
    std::vector<FrameDurationDrag> draggedFrameStartDurations{};
    float draggedFrameStartMouseX{};
    float draggedFrameWidth{};
    bool isDraggedFrameSnapshot{};
    bool frameFocusRequested{};
    int frameFocusIndex{-1};
    FrameMoveDrag frameMoveDrag{};
    std::vector<int> frameSelectionSnapshot{};
    std::vector<int> frameSelectionLocked{};
    bool isFrameSelectionLocked{};
    Reference frameSelectionSnapshotReference{};
    Reference frameSelectionAnchor{};
    bool isFrameSelectionAnchorSet{};
    std::vector<TimelineRowReference> rowDragReferences{};
    glm::vec2 scroll{};
    ImGuiStyle style{};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
