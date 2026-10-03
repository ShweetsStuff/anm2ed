#pragma once

#include <memory>
#include <optional>
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
  struct TimelineContext;

  // A timeline row: a track (in a group or not), a group, or a group's root. groupType/groupId name the group a
  // track or group root belongs to.
  struct TimelineRow
  {
    int documentIndex{-1};
    int animationIndex{-1};
    int type{NONE};
    int id{-1};
    int index{-1};
    int groupType{NONE};
    int groupId{-1};
    int depth{};
    bool isGroup{};

    auto operator<=>(const TimelineRow&) const = default;
  };

  struct FrameMoveDrag
  {
    int animationIndex{-1};
    int duration{1};
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

  struct TimelineState
  {
    bool isDragging{};
    bool isWindowHovered{};
    bool isHorizontalScroll{};
    popup::ItemProperties itemProperties{};
    PopupHelper bakePopup{PopupHelper(LABEL_TIMELINE_BAKE_POPUP, POPUP_SMALL_NO_HEIGHT)};
    PopupHelper bakeIntoOtherFramesPopup{PopupHelper(LABEL_BAKE_INTO_OTHER_FRAMES, POPUP_SMALL_NO_HEIGHT)};
    PopupHelper makeManyRegionsPopup{PopupHelper(LABEL_MAKE_MANY_REGIONS, POPUP_SMALL_NO_HEIGHT)};
    BakeIntoOtherFramesTarget bakeIntoOtherFramesTarget{BakeIntoOtherFramesTarget::CURRENT_SELECTION};
    bool isBakeIntoOtherFramesLayers{true};
    bool isBakeIntoOtherFramesNulls{true};
    bool isMakeManyRegionsMapFrames{true};
    std::set<Reference> makeManyRegionReferences{};
    int hoveredTime{};
    bool isFrameBoxPending{};
    bool isFrameBoxSelecting{};
    bool isFrameBoxAdditive{};
    ImVec2 frameBoxStart{};
    ImVec2 frameBoxEnd{};
    std::set<Reference> frameBoxSelection{};
    std::optional<TimelineRow> rowSelectionAnchor{};
    PopupHelper groupPropertiesPopup{PopupHelper(LABEL_GROUP_PROPERTIES, POPUP_SMALL_NO_HEIGHT)};
    std::string groupName{};
    int groupAnimationIndex{-1};
    int groupType{NONE};
    int groupId{-1};
    Reference draggedFrameReference{};
    bool isDraggedFrameActive{};
    int draggedFrameType{};
    int draggedFrameStart{-1};
    int draggedFrameStartDuration{-1};
    std::vector<FrameDurationDrag> draggedFrameStartDurations{};
    float draggedFrameStartMouseX{};
    float draggedFrameWidth{};
    bool isDraggedFrameSnapshot{};
    FrameMoveDrag frameMoveDrag{};
    std::optional<Reference> frameSelectionAnchor{};
    int animationLengthEditIndex{-1};
    std::vector<TimelineRow> rowDragReferences{};
    glm::vec2 scroll{};
    ImGuiStyle style{};
  };

  class Timeline
  {
    std::unique_ptr<TimelineContext> context{};

  public:
    Timeline();
    ~Timeline();
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
