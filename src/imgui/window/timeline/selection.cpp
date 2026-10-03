#include "context.hpp"

namespace anm2ed::imgui
{
  const model::Track* TimelineContext::item_get(int type, int id, int groupType, int groupId)
  {
    return animation ? model::animation_track_get(*animation, type, id, groupType, groupId) : nullptr;
  }

  const model::TrackGroup* TimelineContext::track_group_get(int type, int groupId)
  {
    return animation ? model::animation_track_group_get(*animation, type, groupId) : nullptr;
  }

  bool TimelineContext::is_track_group_visible(int type, int groupId)
  {
    if (groupId == -1) return true;
    auto group = track_group_get(type, groupId);
    return !group || group->isVisible;
  }

  const model::TrackGroup* TimelineContext::row_group_get(const TimelineRow& row)
  {
    return row.isGroup ? track_group_get(row.type, row.id) : nullptr;
  }

  const model::Frame* TimelineContext::frame_get() { return document.model.frame_get(reference); }

  const model::Track* TimelineContext::selected_item_get() { return document.model.track_get(reference); }

  glm::vec4 TimelineContext::color_get(TimelineColor color, int type)
  {
    auto& colors = TIMELINE_COLORS[color];
    return isLightTheme ? colors.light[std::clamp(type, 0, TRIGGER)] : colors.dark[type];
  }

  std::set<Reference> TimelineContext::drag_frame_references_get(const Reference& frameReference)
  {
    auto selectedReferences = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (!selectedReferences.contains(frameReference)) selectedReferences = {frameReference};
    std::erase_if(selectedReferences, is_trigger_reference);
    return selectedReferences.empty() ? std::set<Reference>{frameReference} : selectedReferences;
  }

  void TimelineContext::edit_begin_push(StringType label)
  {
    command_push([label](Manager&, Document& document) { document.edit_begin(label); });
  }

  void TimelineContext::reference_set(Reference value)
  {
    reference = value;
    document.reference_set(value);
  }

  Reference TimelineContext::item_reference_get(int type, int id, int groupType, int groupId)
  {
    return Reference{reference.animationIndex, type, id, -1, groupType, groupId};
  }

  Reference TimelineContext::item_reference_from_frame_get(Reference frameReference)
  {
    frameReference.frameIndex = -1;
    return frameReference;
  }

  bool TimelineContext::is_same_item(const Reference& left, const Reference& right)
  {
    return left.animationIndex == right.animationIndex && left.itemType == right.itemType &&
           left.itemID == right.itemID && left.groupType == right.groupType && left.groupId == right.groupId;
  }

  void TimelineContext::group_selection_reset_for(Document& targetDocument)
  {
    targetDocument.selection.uids[SelectionKind::GROUPS].clear();
    rowSelectionAnchor.reset();
  }

  std::set<Reference> TimelineContext::item_references_for_current_get()
  {
    auto result = document.selected_get(SelectionKind::TRACKS);
    if (result.empty() && reference.itemType != NONE) result.insert(item_reference_from_frame_get(reference));
    return result;
  }

  std::set<Reference> TimelineContext::copy_frame_references_get()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (!selectedFrames.empty()) return selectedFrames;

    auto selectedItems = document.selected_get(SelectionKind::TRACKS);
    if (selectedItems.empty() && reference.itemType != NONE && reference.frameIndex < 0)
      selectedItems.insert(item_reference_from_frame_get(reference));

    std::set<Reference> result{};
    for (auto itemReference : selectedItems)
    {
      itemReference.frameIndex = -1;
      if (itemReference.itemType == TRIGGER) continue;
      auto itemFrames = document.item_frame_references_get(itemReference);
      result.insert(itemFrames.begin(), itemFrames.end());
    }
    return result;
  }

  void TimelineContext::command_push(std::function<void(Manager&, Document&)> run)
  {
    manager.command_push(
        {manager.selected, [run](Manager& manager, Document& document) mutable { run(manager, document); }});
  }

  std::vector<TimelineRow> TimelineContext::timeline_item_rows_get()
  {
    std::vector<TimelineRow> rows{};
    if (!animation) return rows;

    auto is_track_shown = [&](const model::Track& track)
    { return settings.timelineIsShowUnused || !track.frames.empty(); };
    // Layers are listed top-down in reverse file order; nulls in file order.
    auto order_get = [](int count, bool isReversed)
    {
      std::vector<int> order(count);
      for (int i = 0; i < count; ++i)
        order[i] = isReversed ? count - 1 - i : i;
      return order;
    };

    rows.push_back({.type = ROOT});
    for (auto type : {LAYER, NULL_})
    {
      auto& entries = model::animation_entries_get(*animation, type);
      auto isReversed = type == LAYER;
      for (auto index : order_get((int)entries.size(), isReversed))
      {
        if (auto track = std::get_if<model::Track>(&entries[index]))
        {
          if (is_track_shown(*track)) rows.push_back({.type = type, .id = track->id, .index = index});
          continue;
        }
        auto& group = std::get<model::TrackGroup>(entries[index]);
        rows.push_back({.type = type, .id = group.id, .index = index, .isGroup = true});
        if (!group.isExpanded) continue;
        rows.push_back({.type = ROOT, .groupType = type, .groupId = group.id, .depth = 1});
        for (auto trackIndex : order_get((int)group.tracks.size(), isReversed))
          if (auto& track = group.tracks[trackIndex]; is_track_shown(track))
            rows.push_back({.type = type,
                            .id = track.id,
                            .index = trackIndex,
                            .groupType = type,
                            .groupId = group.id,
                            .depth = 1});
      }
    }

    rows.push_back({.type = TRIGGER});
    for (auto& row : rows)
    {
      row.documentIndex = manager.selected;
      row.animationIndex = reference.animationIndex;
    }
    return rows;
  }

  bool TimelineContext::is_row_selected(const TimelineRow& row)
  {
    if (row.isGroup) return is_row_reference_selected(row);
    auto itemReference = item_reference_get(row.type, row.id, row.groupType, row.groupId);
    auto tracks = document.selected_get(SelectionKind::TRACKS);
    return tracks.contains(itemReference) ||
           (tracks.empty() && document.selection.uids[SelectionKind::GROUPS].empty() &&
            is_same_item(reference, itemReference));
  }

  Reference row_item_reference_get(const TimelineRow& row)
  {
    return Reference{row.animationIndex, row.type, row.id, -1, row.groupType, row.groupId};
  }

  Reference row_selection_reference_get(const TimelineRow& row)
  {
    return row.isGroup ? Reference{row.animationIndex, row.type, row.id} : row_item_reference_get(row);
  }

  SelectionKind row_kind_get(const TimelineRow& row)
  {
    return row.isGroup ? SelectionKind::GROUPS : SelectionKind::TRACKS;
  }

  void TimelineContext::row_selection_clear()
  {
    document.selection.uids[SelectionKind::TRACKS].clear();
    document.selection.uids[SelectionKind::GROUPS].clear();
  }

  void TimelineContext::row_selection_insert(const TimelineRow& row)
  {
    auto references = document.selected_get(row_kind_get(row));
    references.insert(row_selection_reference_get(row));
    document.selected_set(row_kind_get(row), references);
  }

  bool TimelineContext::is_row_reference_selected(const TimelineRow& row)
  {
    return document.selected_get(row_kind_get(row)).contains(row_selection_reference_get(row));
  }

  void TimelineContext::row_selection_set(const TimelineRow& row)
  {
    auto rowReference = row;
    auto isCtrlDown = ImGui::IsKeyDown(ImGuiMod_Ctrl);
    auto isShiftDown = ImGui::IsKeyDown(ImGuiMod_Shift);

    if (row.isGroup)
      reference_set({reference.animationIndex});
    else
    {
      if (row.type == LAYER)
        if (auto layer = model::item_get(model.content.layers, row.id))
          document.focused_id_set(SelectionKind::SPRITESHEETS, layer->spritesheetId);
      reference_set(row_item_reference_get(rowReference));
    }
    document.frame_references_clear();

    if (isShiftDown)
    {
      auto rowSelection = timeline_item_rows_get();
      auto anchor = rowSelectionAnchor.value_or(rowReference);
      auto first = std::find(rowSelection.begin(), rowSelection.end(), anchor);
      auto last = std::find(rowSelection.begin(), rowSelection.end(), rowReference);
      if (first == rowSelection.end() || last == rowSelection.end())
      {
        if (!isCtrlDown) row_selection_clear();
        row_selection_insert(rowReference);
        rowSelectionAnchor = rowReference;
      }
      else
      {
        auto firstIndex = (int)std::distance(rowSelection.begin(), first);
        auto lastIndex = (int)std::distance(rowSelection.begin(), last);
        if (firstIndex > lastIndex) std::swap(firstIndex, lastIndex);
        if (!isCtrlDown) row_selection_clear();
        for (int i = firstIndex; i <= lastIndex; ++i)
          row_selection_insert(rowSelection[i]);
        if (!rowSelectionAnchor) rowSelectionAnchor = rowReference;
      }
    }
    else if (isCtrlDown)
    {
      if (is_row_reference_selected(rowReference) && document.selection.uids[SelectionKind::TRACKS].size() +
                                                             document.selection.uids[SelectionKind::GROUPS].size() >
                                                         1)
      {
        auto references = document.selected_get(row_kind_get(rowReference));
        references.erase(row_selection_reference_get(rowReference));
        document.selected_set(row_kind_get(rowReference), references);
      }
      else
        row_selection_insert(rowReference);
      rowSelectionAnchor = rowReference;
    }
    else
    {
      row_selection_clear();
      row_selection_insert(rowReference);
      rowSelectionAnchor = rowReference;
    }
  }

  void TimelineContext::reference_set_adjacent_item(int direction)
  {
    std::vector<Reference> itemReferences{};
    for (const auto& row : timeline_item_rows_get())
      if (!row.isGroup) itemReferences.push_back(row_item_reference_get(row));
    if (itemReferences.empty()) return;

    auto it = std::find_if(itemReferences.begin(), itemReferences.end(),
                           [&](const Reference& itemReference) { return is_same_item(itemReference, reference); });

    int index = direction > 0 ? 0 : (int)itemReferences.size() - 1;
    if (it != itemReferences.end()) index = (int)std::distance(itemReferences.begin(), it) + direction;
    index = std::clamp(index, 0, (int)itemReferences.size() - 1);

    auto& itemReference = itemReferences[index];
    group_selection_reset_for(document);
    document.track_select(itemReference);
  }

  std::vector<TimelineRow> TimelineContext::selected_row_references_get()
  {
    std::vector<TimelineRow> result{};
    for (const auto& row : timeline_item_rows_get())
      if (is_row_selected(row)) result.push_back(row);
    return result;
  }

  std::vector<TimelineRow> TimelineContext::row_drag_references_get(const TimelineRow& row)
  {
    auto clicked = row;
    if (clicked.type != LAYER && clicked.type != NULL_) return std::vector<TimelineRow>{clicked};

    auto rows = selected_row_references_get();
    if (!is_row_reference_selected(clicked)) rows = {clicked};
    if (rows.empty()) rows = {clicked};

    std::erase_if(rows, [&](const TimelineRow& rowReference) { return rowReference.type != clicked.type; });
    if (rows.empty()) rows = {clicked};
    return rows;
  }
}
