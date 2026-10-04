#pragma once

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "clipboard.hpp"
#include "icon.hpp"
#include "manager.hpp"
#include "popup/item_properties.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "strings.hpp"
#include "util/imgui/popup.hpp"

namespace anm2ed::imgui
{
  inline const glm::vec4 ROOT_COLOR = glm::vec4(0.140f, 0.310f, 0.560f, 1.000f);
  inline const glm::vec4 ROOT_COLOR_ACTIVE = glm::vec4(0.240f, 0.520f, 0.880f, 1.000f);
  inline const glm::vec4 ROOT_COLOR_HOVERED = glm::vec4(0.320f, 0.640f, 1.000f, 1.000f);

  inline const glm::vec4 LAYER_COLOR = glm::vec4(0.640f, 0.320f, 0.110f, 1.000f);
  inline const glm::vec4 LAYER_COLOR_ACTIVE = glm::vec4(0.840f, 0.450f, 0.170f, 1.000f);
  inline const glm::vec4 LAYER_COLOR_HOVERED = glm::vec4(0.960f, 0.560f, 0.240f, 1.000f);

  inline const glm::vec4 NULL_COLOR = glm::vec4(0.140f, 0.430f, 0.200f, 1.000f);
  inline const glm::vec4 NULL_COLOR_ACTIVE = glm::vec4(0.250f, 0.650f, 0.350f, 1.000f);
  inline const glm::vec4 NULL_COLOR_HOVERED = glm::vec4(0.350f, 0.800f, 0.480f, 1.000f);

  inline const glm::vec4 TRIGGER_COLOR = glm::vec4(0.620f, 0.150f, 0.260f, 1.000f);
  inline const glm::vec4 TRIGGER_COLOR_ACTIVE = glm::vec4(0.820f, 0.250f, 0.380f, 1.000f);
  inline const glm::vec4 TRIGGER_COLOR_HOVERED = glm::vec4(0.950f, 0.330f, 0.490f, 1.000f);

#define ANM2_ITEM_TYPES                                                                                                \
  X(NONE, STRING_UNDEFINED, resource::icon::NONE, glm::vec4(), glm::vec4(), glm::vec4())                               \
  X(ROOT, BASIC_ROOT, resource::icon::ROOT, ROOT_COLOR, ROOT_COLOR_ACTIVE, ROOT_COLOR_HOVERED)                         \
  X(LAYER, BASIC_LAYER_ANIMATION, resource::icon::LAYER, LAYER_COLOR, LAYER_COLOR_ACTIVE, LAYER_COLOR_HOVERED)         \
  X(NULL_, BASIC_NULL_ANIMATION, resource::icon::NULL_, NULL_COLOR, NULL_COLOR_ACTIVE, NULL_COLOR_HOVERED)             \
  X(TRIGGER, BASIC_TRIGGERS, resource::icon::TRIGGERS, TRIGGER_COLOR, TRIGGER_COLOR_ACTIVE, TRIGGER_COLOR_HOVERED)

  constexpr StringType TYPE_STRINGS[] = {
#define X(symbol, string, icon, color, colorActive, colorHovered) string,
      ANM2_ITEM_TYPES
#undef X
  };

  constexpr resource::icon::Type TYPE_ICONS[] = {
#define X(symbol, string, icon, color, colorActive, colorHovered) icon,
      ANM2_ITEM_TYPES
#undef X
  };

  inline const glm::vec4 TYPE_COLOR[] = {
#define X(symbol, string, icon, color, colorActive, colorHovered) color,
      ANM2_ITEM_TYPES
#undef X
  };

  inline const glm::vec4 TYPE_COLOR_ACTIVE[] = {
#define X(symbol, string, icon, color, colorActive, colorHovered) colorActive,
      ANM2_ITEM_TYPES
#undef X
  };

  inline const glm::vec4 TYPE_COLOR_HOVERED[] = {
#define X(symbol, string, icon, color, colorActive, colorHovered) colorHovered,
      ANM2_ITEM_TYPES
#undef X
  };

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

  // What the timeline last put on the clipboard: frames of a type, or one property of a frame.
  struct FrameClipboard
  {
    std::string text{};
    int itemType{NONE};
    int property{-1};
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
    bool isRulerHovered{};
    FrameClipboard frameClipboard{};
    int markerTime{};
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
    // The tab a held shorten/extend chord is editing (0 when released), so a held chord starts one undo entry per document.
    std::uint64_t resizeChordTabIds[2]{};
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
