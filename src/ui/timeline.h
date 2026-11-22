#pragma once

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "clipboard.h"
#include "icon.h"
#include "manager.h"
#include "ui/item_properties.h"
#include "resources.h"
#include "settings.h"
#include "localization.h"
#include "ui/popup.h"

constexpr Vec4 ROOT_COLOR = Vec4(0.140f, 0.310f, 0.560f, 1.000f);

// How each item type shows in the timeline: its name, icon, and color (normal, active and hovered).
struct ItemTypeStyle
{
    StringType label;
    Icon icon;
    Vec4 colors[3];
};

constexpr ItemTypeStyle ITEM_TYPE_STYLES[] = {
    {STRING_UNDEFINED, ICON_NONE, {}},
    {BASIC_ROOT, ICON_ROOT, {ROOT_COLOR, {0.240f, 0.520f, 0.880f, 1.000f}, {0.320f, 0.640f, 1.000f, 1.000f}}},
    {BASIC_LAYER_ANIMATION,
     ICON_LAYER,
     {{0.640f, 0.320f, 0.110f, 1.000f}, {0.840f, 0.450f, 0.170f, 1.000f}, {0.960f, 0.560f, 0.240f, 1.000f}}},
    {BASIC_NULL_ANIMATION,
     ICON_NULL,
     {{0.140f, 0.430f, 0.200f, 1.000f}, {0.250f, 0.650f, 0.350f, 1.000f}, {0.350f, 0.800f, 0.480f, 1.000f}}},
    {BASIC_TRIGGERS,
     ICON_TRIGGERS,
     {{0.620f, 0.150f, 0.260f, 1.000f}, {0.820f, 0.250f, 0.380f, 1.000f}, {0.950f, 0.330f, 0.490f, 1.000f}}}};

struct TimelineContext;

// A timeline row: a track (in a group or not), a group, or a group's root. groupType/groupId name the group a
// track or group root belongs to.
struct TimelineRow
{
    int documentIndex = -1;
    int animationIndex = -1;
    int type = NONE;
    int id = -1;
    int index = -1;
    int groupType = NONE;
    int groupId = -1;
    int depth = 0;
    bool isGroup = false;

    auto operator<=>(const TimelineRow&) const = default;
};

struct FrameMoveDrag
{
    int animationIndex = -1;
    int duration = 1;
    std::vector<Reference> references;
    bool isActive = false;
};

// What the timeline last put on the clipboard: frames of a type, or one property of a frame.
struct FrameClipboard
{
    std::string text;
    int itemType = NONE;
    int property = -1;
};

struct FrameDurationDrag
{
    Reference reference = {};
    int duration = 1;
};

enum class BakeIntoOtherFramesTarget
{
    CURRENT_SELECTION,
    ALL
};

struct TimelineState
{
    bool isDragging = false;
    bool isRulerHovered = false;
    FrameClipboard frameClipboard = {};
    int markerTime = 0;
    bool isWindowHovered = false;
    bool isHorizontalScroll = false;
    ItemProperties itemProperties = {};
    PopupHelper bakePopup = PopupHelper(LABEL_TIMELINE_BAKE_POPUP, POPUP_SMALL_NO_HEIGHT);
    PopupHelper bakeIntoOtherFramesPopup = PopupHelper(LABEL_BAKE_INTO_OTHER_FRAMES, POPUP_SMALL_NO_HEIGHT);
    PopupHelper makeManyRegionsPopup = PopupHelper(LABEL_MAKE_MANY_REGIONS, POPUP_SMALL_NO_HEIGHT);
    BakeIntoOtherFramesTarget bakeIntoOtherFramesTarget = BakeIntoOtherFramesTarget::CURRENT_SELECTION;
    bool isBakeIntoOtherFramesLayers = true;
    bool isBakeIntoOtherFramesNulls = true;
    bool isMakeManyRegionsMapFrames = true;
    std::set<Reference> makeManyRegionReferences;
    int hoveredTime = 0;
    bool isFrameBoxPending = false;
    bool isFrameBoxSelecting = false;
    bool isFrameBoxAdditive = false;
    ImVec2 frameBoxStart;
    ImVec2 frameBoxEnd;
    std::set<Reference> frameBoxSelection;
    std::optional<TimelineRow> rowSelectionAnchor;
    PopupHelper groupPropertiesPopup = PopupHelper(LABEL_GROUP_PROPERTIES, POPUP_SMALL_NO_HEIGHT);
    std::string groupName;
    int groupAnimationIndex = -1;
    int groupType = NONE;
    int groupId = -1;
    Reference draggedFrameReference = {};
    bool isDraggedFrameActive = false;
    int draggedFrameType = 0;
    int draggedFrameStart = -1;
    int draggedFrameStartDuration = -1;
    std::vector<FrameDurationDrag> draggedFrameStartDurations;
    float draggedFrameStartMouseX = 0;
    float draggedFrameWidth = 0;
    bool isDraggedFrameSnapshot = false;
    FrameMoveDrag frameMoveDrag = {};
    // The tab a held shorten/extend chord is editing (0 when released), so a held chord starts one undo entry per
    // document.
    std::uint64_t resizeChordTabIds[2]{};
    std::optional<Reference> frameSelectionAnchor;
    int animationLengthEditIndex = -1;
    std::vector<TimelineRow> rowDragReferences;
    std::set<std::uint64_t> expandedLayerTracks; // animation layer tracks (by uid) showing their child's tracks
    Vec2 scroll{};
    ImGuiStyle style = {};
};

class Timeline
{
    std::unique_ptr<TimelineContext> context;

public:
    Timeline();
    ~Timeline();
    void update(Manager&, Settings&, Resources&, Clipboard&);
};
