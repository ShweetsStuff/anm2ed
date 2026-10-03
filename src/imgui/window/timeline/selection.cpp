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

  const model::TrackGroup* TimelineContext::row_group_get(const TimelineItemRow& row)
  {
    return row.isGroup ? track_group_get(row.type, row.id) : nullptr;
  }

  const model::Track* TimelineContext::command_item_reference_get(Document& document, Reference itemReference)
  {
    return document.model.track_get(itemReference);
  }

  const model::Frame* TimelineContext::command_frame_get(Document& document, const Reference& targetReference)
  {
    return document.model.frame_get(targetReference);
  }

  const model::Frame* TimelineContext::frame_get() { return command_frame_get(document, reference); }

  const model::Track* TimelineContext::selected_item_get() { return command_item_reference_get(document, reference); }

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

  void TimelineContext::frames_select_for(Document& targetDocument, const edit::Uids& uids)
  {
    auto references = targetDocument.references_get(uids);
    if (references.empty()) return;
    auto focus = references.front();
    targetDocument.editTarget = Document::EditTarget::FRAME;
    targetDocument.frame_references_set({references.begin(), references.end()});
    targetDocument.reference_set(focus);
    if (auto item = command_item_reference_get(targetDocument, focus); item && focus.itemType != TRIGGER)
      targetDocument.frameTime = model::frame_time_from_index_get(*item, focus.frameIndex);
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
    isRowSelectionAnchorSet = false;
  }

  std::set<Reference> TimelineContext::item_references_for_current_get()
  {
    auto result = document.selected_get(SelectionKind::TRACKS);
    if (result.empty() && reference.itemType != NONE) result.insert(item_reference_from_frame_get(reference));
    return result;
  }

  void TimelineContext::item_selection_set_for(Document& targetDocument, Reference itemReference)
  {
    targetDocument.selected_set(SelectionKind::TRACKS, {item_reference_from_frame_get(itemReference)});
  }

  void TimelineContext::frame_selection_set_for(Document& targetDocument, Reference frameReference)
  {
    group_selection_reset_for(targetDocument);
    targetDocument.editTarget = Document::EditTarget::FRAME;
    targetDocument.frame_references_set({frameReference});
    targetDocument.reference_set(frameReference);
  }

  void TimelineContext::frame_selection_toggle_for(Document& targetDocument, Reference frameReference)
  {
    group_selection_reset_for(targetDocument);
    targetDocument.editTarget = Document::EditTarget::FRAME;
    auto selection = targetDocument.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (!selection.contains(frameReference))
      selection.insert(frameReference);
    else if (selection.size() > 1)
      selection.erase(frameReference);
    targetDocument.frame_references_set(selection);
    targetDocument.reference_set(frameReference);
  }

  bool TimelineContext::frame_selection_range_set_for(Document& targetDocument, Reference firstReference,
                                                      Reference lastReference, bool isAdditive)
  {
    group_selection_reset_for(targetDocument);
    targetDocument.editTarget = Document::EditTarget::FRAME;
    if (!is_same_item(firstReference, lastReference) || firstReference.frameIndex < 0 || lastReference.frameIndex < 0)
      return false;

    auto item = command_item_reference_get(targetDocument, lastReference);
    if (!item || std::max(firstReference.frameIndex, lastReference.frameIndex) >= model::track_frames_count_get(*item))
      return false;

    auto [firstIndex, lastIndex] = std::minmax(firstReference.frameIndex, lastReference.frameIndex);
    auto selectedFrames = isAdditive ? targetDocument.frame_references_get(Document::FrameReferenceFallback::NONE)
                                     : std::set<Reference>{};
    for (int i = firstIndex; i <= lastIndex; ++i)
    {
      auto frameReference = lastReference;
      frameReference.frameIndex = i;
      selectedFrames.insert(frameReference);
    }

    targetDocument.frame_references_set(selectedFrames);
    targetDocument.reference_set(lastReference);
    return true;
  }

  bool TimelineContext::is_frame_copy_item(const Reference& itemReference)
  {
    return itemReference.itemType == ROOT || itemReference.itemType == LAYER || itemReference.itemType == NULL_;
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
      if (!is_frame_copy_item(itemReference)) continue;
      auto itemFrames = document.item_frame_references_get(itemReference);
      result.insert(itemFrames.begin(), itemFrames.end());
    }
    return result;
  }

  void TimelineContext::frames_selection_reset_for(Document& targetDocument)
  {
    targetDocument.frame_references_clear();
  }

  void TimelineContext::frames_selection_set_reference_for(Document& targetDocument)
  {
    auto targetReference = targetDocument.reference_get();
    if (targetReference.frameIndex < 0) return targetDocument.frame_references_clear();
    targetDocument.editTarget = Document::EditTarget::FRAME;
    targetDocument.frame_references_set({targetReference});
  }

  void TimelineContext::reference_clear_for(Document& targetDocument)
  {
    targetDocument.reference_set({targetDocument.reference_get().animationIndex});
    frames_selection_reset_for(targetDocument);
    targetDocument.selection.uids[SelectionKind::TRACKS].clear();
  }

  void TimelineContext::reference_set_item_reference_for(Document& targetDocument, Reference itemReference)
  {
    itemReference.frameIndex = -1;
    targetDocument.reference_set(itemReference);
    frames_selection_reset_for(targetDocument);
    item_selection_set_for(targetDocument, itemReference);
  }

  void TimelineContext::reference_set_timeline_item_reference_for(Document& targetDocument, Reference itemReference)
  {
    if (itemReference.itemType == LAYER)
      if (auto layer = model::item_get(targetDocument.model.content.layers, itemReference.itemID))
        targetDocument.spritesheet.reference = layer->spritesheetId;
    reference_set_item_reference_for(targetDocument, itemReference);
  }

  void TimelineContext::command_push(std::function<void(Manager&, Document&)> run)
  {
    manager.command_push(
        {manager.selected, [run](Manager& manager, Document& document) mutable { run(manager, document); }});
  }

  void TimelineContext::reference_clear()
  {
    group_selection_reset_for(document);
    reference_clear_for(document);
  }

  void TimelineContext::reference_set_timeline_item_reference(Reference itemReference)
  {
    group_selection_reset_for(document);
    reference_set_timeline_item_reference_for(document, itemReference);
  }

  std::vector<TimelineItemRow> TimelineContext::timeline_item_rows_get()
  {
    std::vector<TimelineItemRow> rows{};
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
        rows.push_back({.type = ROOT, .id = -1, .rootGroupType = type, .rootGroupId = group.id, .depth = 1});
        for (auto trackIndex : order_get((int)group.tracks.size(), isReversed))
          if (auto& track = group.tracks[trackIndex]; is_track_shown(track))
            rows.push_back({.type = type,
                            .id = track.id,
                            .index = trackIndex,
                            .groupId = group.id,
                            .rootGroupType = type,
                            .rootGroupId = group.id,
                            .depth = 1});
      }
    }

    rows.push_back({.type = TRIGGER});
    return rows;
  }

  std::vector<Reference> TimelineContext::timeline_item_references_get()
  {
    std::vector<Reference> itemReferences;
    for (const auto& row : timeline_item_rows_get())
    {
      if (row.isGroup) continue;
      itemReferences.push_back({reference.animationIndex, row.type, row.id, -1, row.rootGroupType, row.rootGroupId});
    }
    return itemReferences;
  }

  Reference TimelineContext::group_reference_get(const TimelineItemRow& row)
  {
    return Reference{reference.animationIndex, row.type, row.id};
  }

  TimelineRowReference TimelineContext::row_reference_get(const TimelineItemRow& row)
  {
    return TimelineRowReference{manager.selected, reference.animationIndex, row.type,        row.id,
                                row.index,        row.rootGroupType,        row.rootGroupId, row.isGroup};
  }

  Reference TimelineContext::row_item_reference_get(const TimelineRowReference& row)
  {
    return Reference{row.animationIndex, row.type, row.id, -1, row.groupType, row.groupId};
  }

  std::vector<TimelineRowReference> TimelineContext::timeline_row_references_get()
  {
    std::vector<TimelineRowReference> rowReferences;
    for (const auto& row : timeline_item_rows_get())
    {
      if (row.type == NONE) continue;
      rowReferences.push_back(row_reference_get(row));
    }
    return rowReferences;
  }

  bool TimelineContext::is_group_selected(const TimelineItemRow& row)
  {
    return document.selected_get(SelectionKind::GROUPS).contains(group_reference_get(row));
  }

  bool TimelineContext::is_row_selected(const TimelineItemRow& row)
  {
    if (row.isGroup) return is_group_selected(row);
    auto itemReference = item_reference_get(row.type, row.id, row.rootGroupType, row.rootGroupId);
    auto tracks = document.selected_get(SelectionKind::TRACKS);
    return tracks.contains(itemReference) ||
           (tracks.empty() && document.selection.uids[SelectionKind::GROUPS].empty() &&
            is_same_item(reference, itemReference));
  }

  SelectionKind row_kind_get(const TimelineRowReference& row)
  {
    return row.isGroup ? SelectionKind::GROUPS : SelectionKind::TRACKS;
  }

  Reference row_selection_reference_get(const TimelineRowReference& row)
  {
    return row.isGroup ? Reference{row.animationIndex, row.type, row.id}
                       : Reference{row.animationIndex, row.type, row.id, -1, row.groupType, row.groupId};
  }

  void TimelineContext::row_selection_clear()
  {
    document.selection.uids[SelectionKind::TRACKS].clear();
    document.selection.uids[SelectionKind::GROUPS].clear();
  }

  void TimelineContext::row_selection_insert(const TimelineRowReference& row)
  {
    auto references = document.selected_get(row_kind_get(row));
    references.insert(row_selection_reference_get(row));
    document.selected_set(row_kind_get(row), references);
  }

  void TimelineContext::row_selection_erase(const TimelineRowReference& row)
  {
    auto references = document.selected_get(row_kind_get(row));
    references.erase(row_selection_reference_get(row));
    document.selected_set(row_kind_get(row), references);
  }

  bool TimelineContext::is_row_reference_selected(const TimelineRowReference& row)
  {
    return document.selected_get(row_kind_get(row)).contains(row_selection_reference_get(row));
  }

  std::size_t TimelineContext::row_selection_count_get()
  {
    return document.selection.uids[SelectionKind::TRACKS].size() +
           document.selection.uids[SelectionKind::GROUPS].size();
  }

  void TimelineContext::row_selection_set(const TimelineItemRow& row)
  {
    auto rowReference = row_reference_get(row);
    auto isCtrlDown = ImGui::IsKeyDown(ImGuiMod_Ctrl);
    auto isShiftDown = ImGui::IsKeyDown(ImGuiMod_Shift);

    if (row.isGroup)
      reference_set({reference.animationIndex});
    else
    {
      if (row.type == LAYER)
        if (auto layer = model::item_get(model.content.layers, row.id))
          document.spritesheet.reference = layer->spritesheetId;
      reference_set(row_item_reference_get(rowReference));
    }
    frames_selection_reset_for(document);

    if (isShiftDown)
    {
      auto rowSelection = timeline_row_references_get();
      auto anchor = isRowSelectionAnchorSet ? rowSelectionAnchor : rowReference;
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
        if (!isRowSelectionAnchorSet) rowSelectionAnchor = rowReference;
      }
    }
    else if (isCtrlDown)
    {
      if (is_row_reference_selected(rowReference) && row_selection_count_get() > 1)
        row_selection_erase(rowReference);
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

    isRowSelectionAnchorSet = true;
  }

  void TimelineContext::reference_set_adjacent_item(int direction)
  {
    auto itemReferences = timeline_item_references_get();
    if (itemReferences.empty()) return;

    auto it = std::find_if(itemReferences.begin(), itemReferences.end(),
                           [&](const Reference& itemReference) { return is_same_item(itemReference, reference); });

    int index = direction > 0 ? 0 : (int)itemReferences.size() - 1;
    if (it != itemReferences.end()) index = (int)std::distance(itemReferences.begin(), it) + direction;
    index = std::clamp(index, 0, (int)itemReferences.size() - 1);

    auto& itemReference = itemReferences[index];
    reference_set_timeline_item_reference(itemReference);
  }

  std::vector<TimelineRowReference> TimelineContext::selected_row_references_get()
  {
    std::vector<TimelineRowReference> result{};
    for (const auto& row : timeline_item_rows_get())
      if (is_row_selected(row)) result.push_back(row_reference_get(row));
    return result;
  }

  std::vector<TimelineRowReference> TimelineContext::row_drag_references_get(const TimelineItemRow& row)
  {
    auto clicked = row_reference_get(row);
    if (clicked.type != LAYER && clicked.type != NULL_) return std::vector<TimelineRowReference>{clicked};

    auto rows = selected_row_references_get();
    if (!is_row_reference_selected(clicked)) rows = {clicked};
    if (rows.empty()) rows = {clicked};

    std::erase_if(rows, [&](const TimelineRowReference& rowReference) { return rowReference.type != clicked.type; });
    if (rows.empty()) rows = {clicked};
    return rows;
  }
}
