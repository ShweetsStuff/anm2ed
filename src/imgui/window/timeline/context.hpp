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

#include "model/frames.hpp"
#include "vector.hpp"
#include "window/window.hpp"

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

#define ITEM_CHILD_WIDTH ImGui::GetTextLineHeightWithSpacing() * 12.5

  enum TimelineColor
  {
    COLOR_FRAME_BASE,
    COLOR_FRAME_ACTIVE,
    COLOR_FRAME_HOVERED,
    COLOR_ITEM_BASE,
    COLOR_ITEM_ACTIVE,
    COLOR_ITEM_SELECTED
  };

  struct TimelineColorSet
  {
    const glm::vec4* light;
    const glm::vec4* dark;
  };

  inline const TimelineColorSet TIMELINE_COLORS[] = {{FRAME_COLOR_LIGHT_BASE, TYPE_COLOR},
                                                     {FRAME_COLOR_LIGHT_ACTIVE, TYPE_COLOR_ACTIVE},
                                                     {FRAME_COLOR_LIGHT_HOVERED, TYPE_COLOR_HOVERED},
                                                     {ITEM_COLOR_LIGHT_BASE, TYPE_COLOR},
                                                     {ITEM_COLOR_LIGHT_ACTIVE, TYPE_COLOR_ACTIVE},
                                                     {ITEM_COLOR_LIGHT_SELECTED, TYPE_COLOR_ACTIVE}};

  inline bool is_trigger_reference(const Reference& reference) { return reference.itemType == TRIGGER; }

  constexpr resource::icon::Type INTERPOLATION_ICONS[] = {resource::icon::UNINTERPOLATED, resource::icon::INTERPOLATED,
                                                          resource::icon::EASE_IN, resource::icon::EASE_OUT,
                                                          resource::icon::EASE_IN_OUT};

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

  struct TimelineContext : TimelineState
  {
    Manager& manager;
    Settings& settings;
    Resources& resources;
    Clipboard& clipboard;
    Document& document;
    model::Model& model;
    Playback& playback;
    Reference reference;
    Storage& region;
    const model::Animation* animation{};
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

    const model::Track* item_get(int type, int id = -1, int groupType = NONE, int groupId = -1);
    const model::Frame* frame_get();
    const model::Track* selected_item_get();
    const model::TrackGroup* track_group_get(int type, int groupId);
    bool is_track_group_visible(int type, int groupId);
    const model::TrackGroup* row_group_get(const TimelineItemRow& row);
    const model::Track* command_item_reference_get(Document& document, Reference itemReference);
    glm::vec4 color_get(TimelineColor, int);
    std::set<Reference> drag_frame_references_get(const Reference&);
    // Queues an edit on the document; its result is selected (frames by default).
    template <class Operation>
    void edit_push(StringType label, Operation operation, std::function<void(Document&, const edit::Uids&)> select = {})
    {
      command_push(
          [=, this](Manager&, Document& document) mutable
          {
            auto uids = document.edit_apply(label, operation);
            select ? select(document, uids) : frames_select_for(document, uids);
          });
    }
    void edit_begin_push(StringType);
    void reference_set(Reference);
    void frames_select_for(Document&, const edit::Uids&);
    const model::Frame* command_frame_get(Document& document, const Reference& targetReference);
    Reference item_reference_get(int type, int id, int groupType = NONE, int groupId = -1);
    Reference item_reference_from_frame_get(Reference frameReference);
    bool is_same_item(const Reference& left, const Reference& right);
    void group_selection_reset_for(Document& targetDocument);
    std::set<Reference> item_references_for_current_get();
    void item_selection_set_for(Document& targetDocument, Reference itemReference);
    void frame_selection_set_for(Document& targetDocument, Reference frameReference);
    void frame_selection_toggle_for(Document& targetDocument, Reference frameReference);
    bool frame_selection_range_set_for(Document& targetDocument, Reference firstReference, Reference lastReference,
                                       bool isAdditive);
    bool is_frame_copy_item(const Reference& itemReference);
    std::set<Reference> copy_frame_references_get();
    void frames_selection_reset_for(Document& targetDocument);
    void frames_selection_set_reference_for(Document& targetDocument);
    void reference_clear_for(Document& targetDocument);
    void reference_set_item_reference_for(Document& targetDocument, Reference itemReference);
    void reference_set_timeline_item_reference_for(Document& targetDocument, Reference itemReference);
    void command_push(std::function<void(Manager&, Document&)> run);
    void overlay_icon(GLuint textureId, ImVec4 tint, bool isForced = false);
    void playback_stop();
    void frame_insert();
    void frames_delete_action();
    void frames_duplicate();
    bool is_frames_reverse_available(std::set<Reference> selectedFrames);
    void frames_reverse();
    void frames_bake();
    std::set<Reference> selected_root_frame_references_get();
    std::set<Reference> item_references_for_bake_into_other_frames_get(const Document& targetDocument,
                                                                       const model::Animation& targetAnimation,
                                                                       BakeIntoOtherFramesTarget target, bool isLayers,
                                                                       bool isNulls);
    bool is_bake_into_other_frames_ready();
    void bake_into_other_frames();
    void frame_split();
    void reference_clear();
    void reference_set_timeline_item_reference(Reference itemReference);
    std::vector<TimelineItemRow> timeline_item_rows_get();
    std::vector<Reference> timeline_item_references_get();
    Reference group_reference_get(const TimelineItemRow& row);
    TimelineRowReference row_reference_get(const TimelineItemRow& row);
    Reference row_item_reference_get(const TimelineRowReference& row);
    std::vector<TimelineRowReference> timeline_row_references_get();
    bool is_group_selected(const TimelineItemRow& row);
    bool is_row_selected(const TimelineItemRow& row);
    void row_selection_clear();
    void row_selection_insert(const TimelineRowReference& row);
    void row_selection_erase(const TimelineRowReference& row);
    bool is_row_reference_selected(const TimelineRowReference& row);
    std::size_t row_selection_count_get();
    void row_selection_set(const TimelineItemRow& row);
    void reference_set_adjacent_item(int direction);
    std::vector<TimelineRowReference> selected_row_references_get();
    std::vector<TimelineRowReference> row_drag_references_get(const TimelineItemRow& row);
    void item_remove();
    std::vector<Reference> item_references_groupable_get();
    void item_group();
    void rows_move_to_row(std::vector<TimelineRowReference> draggedRows, TimelineItemRow targetRow, bool isDropAfter,
                          bool isDropIntoGroup = false);
    void fit_animation_length();
    void frame_references_copy(const std::set<Reference>& selectedFrames);
    void copy();
    void cut();
    void paste();
    void context_menu();
    void item_base_properties_open(int type, int id);
    void group_properties_close();
    void group_properties_open(const TimelineItemRow& row, const model::TrackGroup& group);
    void group_properties_update();
    void item_context_menu();

    TimelineContext(TimelineState&&, Manager&, Settings&, Resources&, Clipboard&);
    void update();
    void frame_begin();
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
