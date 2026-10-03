#pragma once

#include "timeline.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <set>

#include <imgui_internal.h>

#include "actions.hpp"
#include "log.hpp"
#include "math.hpp"
#include "toast.hpp"
#include "util/imgui/draw.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"
#include "util/imgui/tooltip.hpp"

#include "vector.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{

  constexpr auto WIDTH_MULTIPLIER = 1.25f;

  constexpr auto COLOR_HIDDEN_MULTIPLIER = vec4(0.5f, 0.5f, 0.5f, 1.000f);
  constexpr auto FRAME_BORDER_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.15f);
  constexpr auto FRAME_BORDER_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.25f);
  constexpr auto FRAME_BORDER_COLOR_REFERENCED_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.50f);
  constexpr auto FRAME_BORDER_COLOR_REFERENCED_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
  constexpr auto FRAME_BORDER_THICKNESS = 1.0f;
  constexpr auto FRAME_BORDER_THICKNESS_REFERENCED = 2.0f;
  constexpr auto FRAME_MULTIPLE_OVERLAY_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.05f);
  constexpr auto FRAME_MULTIPLE_OVERLAY_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.1f);
  constexpr auto FRAME_ROUNDING = 3.0f;
  constexpr auto ICON_TINT_DEFAULT_DARK = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  constexpr auto ICON_TINT_DEFAULT_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto ITEM_TEXT_COLOR_DARK = to_imvec4(color::WHITE);
  constexpr auto ITEM_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto PLAYHEAD_ICON_TINT_LIGHT = ImVec4(0.3922f, 0.7843f, 0.5882f, 1.0f);
  constexpr auto PLAYHEAD_LINE_COLOR_DARK = to_imvec4(color::WHITE);
  constexpr auto PLAYHEAD_LINE_COLOR_LIGHT = ImVec4(0.1961f, 0.5882f, 0.3922f, 1.0f);
  constexpr auto PLAYHEAD_LINE_THICKNESS = 4.0f;
  constexpr auto TEXT_MULTIPLE_COLOR_DARK = to_imvec4(color::WHITE);
  constexpr auto TEXT_MULTIPLE_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto TIMELINE_BACKGROUND_COLOR_LIGHT = ImVec4(0.5490f, 0.5490f, 0.5882f, 1.0f);
  constexpr auto TIMELINE_PLAYHEAD_RECT_COLOR_DARK = ImVec4(0.60f, 0.45f, 0.30f, 1.0f);
  constexpr auto TIMELINE_PLAYHEAD_RECT_COLOR_LIGHT = ImVec4(0.8353f, 0.8353f, 0.7294f, 1.0f);
  constexpr auto TIMELINE_TICK_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto TIMELINE_CHILD_BG_COLOR_LIGHT = ImVec4(0.5490f, 0.5490f, 0.5882f, 1.0f);
  constexpr auto TIMELINE_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

  enum class DropZone
  {
    BEFORE,
    INSIDE,
    AFTER
  };

  constexpr glm::vec4 FRAME_COLOR_LIGHT_BASE[] = {{0.80f, 0.80f, 0.80f, 1.0f},
                                                  {0.5216f, 0.7333f, 1.0f, 1.0f},
                                                  {1.0f, 0.9961f, 0.5882f, 1.0f},
                                                  {0.6157f, 1.0f, 0.5882f, 1.0f},
                                                  {1.0f, 0.5882f, 0.8314f, 1.0f}};
  constexpr glm::vec4 FRAME_COLOR_LIGHT_ACTIVE[] = {{0.74f, 0.74f, 0.74f, 1.0f},
                                                    {0.0980f, 0.3765f, 0.6431f, 1.0f},
                                                    {1.0f, 0.5255f, 0.3333f, 1.0f},
                                                    {0.3686f, 0.5765f, 0.2353f, 1.0f},
                                                    {0.6118f, 0.2039f, 0.2745f, 1.0f}};
  constexpr glm::vec4 FRAME_COLOR_LIGHT_HOVERED[] = {{0.84f, 0.84f, 0.84f, 1.0f},
                                                     {0.0752f, 0.2887f, 0.4931f, 1.0f},
                                                     {0.85f, 0.4467f, 0.2833f, 1.0f},
                                                     {0.2727f, 0.4265f, 0.1741f, 1.0f},
                                                     {0.4618f, 0.1539f, 0.2072f, 1.0f}};
  constexpr glm::vec4 ITEM_COLOR_LIGHT_BASE[] = {{0.3059f, 0.3255f, 0.5412f, 1.0f},
                                                 {0.3333f, 0.5725f, 0.8392f, 1.0f},
                                                 {1.0f, 0.5412f, 0.3412f, 1.0f},
                                                 {0.5255f, 0.8471f, 0.4588f, 1.0f},
                                                 {0.7961f, 0.3882f, 0.5412f, 1.0f}};
  constexpr glm::vec4 ITEM_COLOR_LIGHT_ACTIVE[] = {{0.3459f, 0.3655f, 0.5812f, 1.0f},
                                                   {0.3733f, 0.6125f, 0.8792f, 1.0f},
                                                   {0.9106f, 0.4949f, 0.2753f, 1.0f},
                                                   {0.5655f, 0.8871f, 0.4988f, 1.0f},
                                                   {0.8361f, 0.4282f, 0.5812f, 1.0f}};
  constexpr glm::vec4 ITEM_COLOR_LIGHT_SELECTED[] = {{0.74f, 0.74f, 0.74f, 1.0f},
                                                     {0.2039f, 0.4549f, 0.7176f, 1.0f},
                                                     {0.8745f, 0.4392f, 0.2275f, 1.0f},
                                                     {0.3765f, 0.6784f, 0.2980f, 1.0f},
                                                     {0.6353f, 0.2235f, 0.3647f, 1.0f}};

  constexpr auto FRAME_MULTIPLE = 5;
  constexpr auto FRAME_TOOLTIP_HOVER_DELAY = 0.75f;
  constexpr auto DRAG_DROP_SOURCE_FLAGS =
      ImGuiDragDropFlags_SourceNoPreviewTooltip | ImGuiDragDropFlags_SourceNoHoldToOpenOthers;

#define ITEM_CHILD_WIDTH ImGui::GetTextLineHeightWithSpacing() * 12.5

  struct TimelineItemRow
  {
    int type{NONE};
    int id{-1};
    int index{-1};
    int groupId{-1};
    int rootGroupType{NONE};
    int rootGroupId{-1};
    int depth{};
    bool isGroup{};
  };

  struct RootFrameSpan
  {
    int start{};
    int end{};
  };

  struct TimelineContext
  {
    Timeline& timeline;
    bool& isDragging;
    bool& isWindowHovered;
    bool& isHorizontalScroll;
    popup::ItemProperties& itemProperties;
    PopupHelper& bakePopup;
    PopupHelper& bakeIntoOtherFramesPopup;
    PopupHelper& makeManyRegionsPopup;
    BakeIntoOtherFramesTarget& bakeIntoOtherFramesTarget;
    bool& isBakeIntoOtherFramesLayers;
    bool& isBakeIntoOtherFramesNulls;
    bool& isMakeManyRegionsMapFrames;
    std::set<Reference>& makeManyRegionReferences;
    int& hoveredTime;
    bool& isFrameBoxPending;
    bool& isFrameBoxSelecting;
    bool& isFrameBoxAdditive;
    ImVec2& frameBoxStart;
    ImVec2& frameBoxEnd;
    std::set<Reference>& frameBoxSelection;
    TimelineRowReference& rowSelectionAnchor;
    bool& isRowSelectionAnchorSet;
    PopupHelper& groupPropertiesPopup;
    std::string& groupName;
    int& groupAnimationIndex;
    int& groupType;
    int& groupId;
    Reference& draggedFrameReference;
    bool& isDraggedFrameActive;
    int& draggedFrameType;
    int& draggedFrameIndex;
    int& draggedFrameStart;
    int& draggedFrameStartDuration;
    std::vector<FrameDurationDrag>& draggedFrameStartDurations;
    float& draggedFrameStartMouseX;
    float& draggedFrameWidth;
    bool& isDraggedFrameSnapshot;
    bool& frameFocusRequested;
    int& frameFocusIndex;
    FrameMoveDrag& frameMoveDrag;
    std::vector<int>& frameSelectionSnapshot;
    std::vector<int>& frameSelectionLocked;
    bool& isFrameSelectionLocked;
    Reference& frameSelectionSnapshotReference;
    Reference& frameSelectionAnchor;
    bool& isFrameSelectionAnchorSet;
    int& animationLengthEditIndex;
    std::vector<TimelineRowReference>& rowDragReferences;
    glm::vec2& scroll;
    ImGuiStyle& style;

    Manager& manager;
    Settings& settings;
    Resources& resources;
    Clipboard& clipboard;
    Document& document;
    Anm2& anm2;
    Playback& playback;
    Reference& reference;
    Storage& frames;
    Storage& region;
    Element* animation{};
    float rowFrameChildHeight{};
    bool isLightTheme{};
    bool isTextPushed{};

    ImVec4 iconTintDefault{};
    ImVec4 itemIconTint{};
    ImVec4 frameBorderColor{};
    ImVec4 frameBorderColorReferenced{};
    ImVec4 frameMultipleOverlayColor{};
    ImVec4 textMultipleColor{};
    ImVec4 playheadLineColor{};
    ImVec4 playheadIconTint{};
    ImVec4 timelineBackgroundColor{};
    ImVec4 timelinePlayheadRectColor{};
    ImVec4 timelineTickColor{};
    ImVec4 itemTextColor{};
    std::optional<int> frameSplitTimeAtCursor{};

    int frameMoveDropType{NONE};
    int frameMoveDropItemID{-1};
    int frameMoveDropGroupType{NONE};
    int frameMoveDropGroupId{-1};
    int frameMoveDropIndex{-1};
    bool isFrameMoveDropTarget{};
    float playheadLineCenterX{};
    float playheadLineTopY{};
    bool isPlayheadLineSet{};
    ImVec2 frameBoxClipMin{};
    ImVec2 frameBoxClipMax{};
    bool isFrameBoxClipSet{};

    std::function<int(int)> type_index{};
    std::function<ItemType(int)> item_type_get{};
    std::function<Element*(int, int, int, int)> item_get{};
    std::function<Element*()> frame_get{};
    std::function<Element*()> selected_item_get{};
    std::function<ElementType(const Element&)> item_frame_type_get{};
    std::function<int(const Element*)> item_frames_count{};
    std::function<int(const Element&, int)> item_frame_child_index_get{};
    std::function<int(const Element&, int)> item_frame_insert_index_get{};
    std::function<Element*(int)> layer_get{};
    std::function<Element*(int)> null_get{};
    std::function<Element*(int)> spritesheet_get{};
    std::function<Element*()> info_get{};
    std::function<ElementType(int)> container_type_get{};
    std::function<ElementType(int)> track_type_get{};
    std::function<int(const Element&, int)> track_id_get{};
    std::function<Element*(int)> track_container_get{};
    std::function<Element*(int, int)> track_group_get{};
    std::function<bool(int, int)> is_track_group_visible{};
    std::function<Element*(const TimelineItemRow&)> row_group_get{};
    std::function<int(int, int)> group_items_count_get{};
    std::function<Element*(Document&, int)> command_animation_get{};
    std::function<Element*(Document&, int, int, int, int, int)> command_item_get{};
    std::function<Element*(Document&, Reference)> command_item_reference_get{};
    std::function<Element*(Document&, const Reference&)> command_frame_get{};
    std::function<Element*(Document&, int)> command_layer_get{};
    std::function<Element*(Document&, int)> command_spritesheet_get{};
    std::function<Element*(Document&)> command_info_get{};
    std::function<Reference(int, int, int, int)> item_reference_get{};
    std::function<Reference(Reference)> item_reference_from_frame_get{};
    std::function<bool(const Reference&, const Reference&)> is_same_item{};
    std::function<bool(Document&, const Reference&)> is_frame_reference_valid_for{};
    std::function<void(Document&)> group_selection_reset_for{};
    std::function<std::set<Reference>()> frame_references_for_current_get{};
    std::function<std::set<Reference>()> item_references_for_current_get{};
    std::function<void(Document&)> frames_selection_sync_for{};
    std::function<void(Document&, Reference)> item_selection_set_for{};
    std::function<void(Document&, Reference)> frame_selection_set_for{};
    std::function<void(Document&, Reference)> frame_selection_toggle_for{};
    std::function<bool(Document&, Reference, Reference, bool)> frame_selection_range_set_for{};
    std::function<std::set<Reference>()> all_frame_references_for_items_get{};
    std::function<bool(const Reference&)> is_frame_copy_item{};
    std::function<std::set<Reference>()> copy_frame_references_get{};
    std::function<void(Document&)> frames_selection_reset_for{};
    std::function<void(Document&)> frames_selection_set_reference_for{};
    std::function<void(Document&)> frames_reference_normalize_for{};
    std::function<void(Document&)> reference_clear_for{};
    std::function<void(Document&, Reference)> reference_set_item_reference_for{};
    std::function<void(Document&, Reference)> reference_set_timeline_item_reference_for{};
    std::function<void(std::function<void(Manager&, Document&)>)> command_push{};
    std::function<std::set<Reference>(std::set<Reference>)> track_references_from_frame_references_get{};
    std::function<void(StringType, const std::set<Reference>&)> tracks_snapshot_command_push{};
    std::function<void(StringType, const std::set<Reference>&)> frames_snapshot_command_push{};
    std::function<void(StringType, Document::ChangeType, const std::set<Reference>&,
                       std::function<void(Manager&, Document&)>)>
        frames_edit_command_push{};
    std::function<void(StringType, Document::ChangeType, const std::set<Reference>&,
                       std::function<void(Manager&, Document&)>)>
        tracks_edit_command_push{};
    std::function<void(StringType, Document::ChangeType, std::function<void(Manager&, Document&)>)> edit_command_push{};
    std::function<glm::vec4(int)> type_color_base_vec{};
    std::function<glm::vec4(int)> type_color_active_vec{};
    std::function<glm::vec4(int)> type_color_hovered_vec{};
    std::function<glm::vec4(int)> item_color_vec{};
    std::function<glm::vec4(int)> item_color_active_vec{};
    std::function<void(GLuint, ImVec4, bool)> overlay_icon{};
    std::function<void()> frames_selection_set_reference{};
    std::function<void()> playback_stop{};
    std::function<void()> frame_insert{};
    std::function<void(Document&, std::set<Reference>)> frames_delete_for{};
    std::function<void()> frames_delete_action{};
    std::function<void()> frames_duplicate{};
    std::function<bool(std::set<Reference>)> is_frames_reverse_available{};
    std::function<void()> frames_reverse{};
    std::function<void()> frames_bake{};
    std::function<std::set<Reference>()> selected_root_frame_references_get{};
    std::function<std::set<Reference>(const Document&, const Element&, BakeIntoOtherFramesTarget, bool, bool)>
        item_references_for_bake_into_other_frames_get{};
    std::function<bool()> is_bake_into_other_frames_ready{};
    std::function<void(Element&, const Element&, bool, bool, bool)> frame_root_transform_apply{};
    std::function<void()> bake_into_other_frames{};
    std::function<void()> frame_split{};
    std::function<void()> reference_clear{};
    std::function<void(Reference)> reference_set_timeline_item_reference{};
    std::function<std::vector<TimelineItemRow>()> timeline_item_rows_get{};
    std::function<std::vector<Reference>()> timeline_item_references_get{};
    std::function<Reference(const TimelineItemRow&)> group_reference_get{};
    std::function<TimelineRowReference(const TimelineItemRow&)> row_reference_get{};
    std::function<Reference(const TimelineRowReference&)> row_item_reference_get{};
    std::function<std::vector<TimelineRowReference>()> timeline_row_references_get{};
    std::function<bool(const TimelineItemRow&)> is_group_selected{};
    std::function<bool(const TimelineItemRow&)> is_row_selected{};
    std::function<void()> row_selection_clear{};
    std::function<void(const TimelineRowReference&)> row_selection_insert{};
    std::function<void(const TimelineRowReference&)> row_selection_erase{};
    std::function<bool(const TimelineRowReference&)> is_row_reference_selected{};
    std::function<std::size_t()> row_selection_count_get{};
    std::function<void(const TimelineItemRow&)> row_selection_set{};
    std::function<void(int)> reference_set_adjacent_item{};
    std::function<std::vector<TimelineRowReference>()> selected_row_references_get{};
    std::function<std::vector<TimelineRowReference>(const TimelineItemRow&)> row_drag_references_get{};
    std::function<void()> item_remove{};
    std::function<std::vector<Reference>()> item_references_groupable_get{};
    std::function<void()> item_group{};
    std::function<void(std::vector<TimelineRowReference>, TimelineItemRow, bool, bool)> rows_move_to_row{};
    std::function<void()> fit_animation_length{};
    std::function<void(const std::set<Reference>&)> frame_references_copy{};
    std::function<void()> copy{};
    std::function<void()> cut{};
    std::function<void()> paste{};
    std::function<void()> context_menu{};
    std::function<void(int, int)> item_base_properties_open{};
    std::function<void()> group_properties_close{};
    std::function<void(const TimelineItemRow&, const Element&)> group_properties_open{};
    std::function<void()> group_properties_update{};
    std::function<void()> item_context_menu{};

    TimelineContext(Timeline&, Manager&, Settings&, Resources&, Clipboard&);
    void update();
    void commands_bind();
    void item_child(const TimelineItemRow&, int);
    void items_child();
    void frame_move_drag_clear();
    void frames_move_to(int, int, int, int, int);
    ImVec2 frame_box_content_point_get();
    ImVec2 frame_box_screen_point_get(ImVec2);
    bool is_frame_box_overlapping(ImVec2, ImVec2, ImVec2, ImVec2);
    void frame_overlay_draw(ImDrawList*, ImVec2, ImVec2);
    void frame_child(const TimelineItemRow&, int&, float);
    void frames_child();
    void draw();
  };
}
