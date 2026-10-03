#include "context.hpp"

namespace anm2ed::imgui
{
  Element* TimelineContext::item_get(int type, int id, int groupType, int groupId)
  {
    return animation ? animation_item_get(*animation, (ItemType)(type), id, groupType, groupId) : nullptr;
  }

  Element* TimelineContext::track_container_get(int type)
  {
    return animation ? child_first_get(*animation, TYPE_CONTAINERS[type]) : nullptr;
  }

  Element* TimelineContext::track_group_get(int type, int groupId)
  {
    auto container = track_container_get(type);
    return container ? child_id_get(*container, ElementType::GROUP, groupId) : nullptr;
  }

  bool TimelineContext::is_track_group_visible(int type, int groupId)
  {
    if (groupId == -1) return true;
    auto group = track_group_get(type, groupId);
    return !group || group->isVisible;
  }

  Element* TimelineContext::row_group_get(const TimelineItemRow& row)
  {
    return row.isGroup ? track_group_get(row.type, row.id) : nullptr;
  }

  int TimelineContext::group_items_count_get(int type, int groupId)
  {
    auto container = track_container_get(type);
    auto trackType = TYPE_TRACKS[type];
    int count{};
    if (!container) return count;
    for (auto& item : container->children)
      if (item.type == trackType && item.groupId == groupId) ++count;
    return count;
  }

  Element* TimelineContext::command_item_reference_get(Document& document, Reference itemReference)
  {
    itemReference.frameIndex = -1;
    return document.anm2.element_get(itemReference);
  }

  Element* TimelineContext::command_frame_get(Document& document, const Reference& targetReference)
  {
    return targetReference.frameIndex < 0 ? nullptr : document.anm2.element_get(targetReference);
  }

  Element* TimelineContext::command_item_get(Document& document, int animationIndex, int type, int id, int groupType,
                                             int groupId)
  {
    return document.anm2.element_get(Reference{animationIndex, type, id, -1, groupType, groupId});
  }

  Element* TimelineContext::frame_get() { return command_frame_get(document, reference); }

  Element* TimelineContext::selected_item_get() { return command_item_reference_get(document, reference); }

  glm::vec4 TimelineContext::color_get(TimelineColor color, int type)
  {
    auto& colors = TIMELINE_COLORS[color];
    return isLightTheme ? colors.light[std::clamp(type, 0, TRIGGER)] : colors.dark[type];
  }

  std::set<Reference> TimelineContext::track_references_from_frame_references_get(std::set<Reference> frameReferences)
  {
    std::set<Reference> result{};
    for (auto frameReference : frameReferences)
      result.insert(item_reference_from_frame_get(frameReference));
    return result;
  }

  void TimelineContext::frames_focus_sync_for(Document& targetDocument)
  {
    frames_selection_sync_for(targetDocument);
    frameSelectionLocked.clear();
    isFrameSelectionLocked = false;
    frameFocusIndex = targetDocument.reference.frameIndex;
    frameFocusRequested = true;
  }

  std::set<Reference> TimelineContext::drag_frame_references_get(const Reference& frameReference)
  {
    auto selectedReferences = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (!selectedReferences.contains(frameReference)) selectedReferences = {frameReference};
    std::erase_if(selectedReferences, is_trigger_reference);
    return selectedReferences.empty() ? std::set<Reference>{frameReference} : selectedReferences;
  }

  void TimelineContext::snapshot_command_push(StringType messageType, SnapshotKind kind,
                                              const std::set<Reference>& references)
  {
    command_push([message = std::string(localize.get(messageType)), kind, references](Manager&, Document& document)
                 { snapshot_take(document, message, kind, references); });
  }

  void TimelineContext::edit_command_push(StringType messageType, Document::ChangeType changeType,
                                          std::function<void(Manager&, Document&)> run, SnapshotKind kind,
                                          const std::set<Reference>& references)
  {
    command_push(
        [message = std::string(localize.get(messageType)), changeType, run, kind, references](Manager& manager,
                                                                                              Document& document)
        {
          snapshot_take(document, message, kind, references);
          run(manager, document);
          document.change(changeType);
        });
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
    targetDocument.groupReferences.clear();
    isRowSelectionAnchorSet = false;
  }

  std::set<Reference> TimelineContext::item_references_for_current_get()
  {
    std::set<Reference> result = document.items.references;
    if (result.empty() && reference.itemType != NONE)
    {
      auto itemReference = reference;
      itemReference.frameIndex = -1;
      result.insert(itemReference);
    }
    return result;
  }

  void TimelineContext::frames_selection_sync_for(Document& targetDocument)
  {
    auto selectedFrames = targetDocument.frame_references_get(Document::FrameReferenceFallback::NONE);
    targetDocument.frame_references_set(std::move(selectedFrames));
    frameSelectionSnapshot.assign(targetDocument.frames.selection.begin(), targetDocument.frames.selection.end());
    frameSelectionSnapshotReference = targetDocument.reference;
  }

  void TimelineContext::item_selection_set_for(Document& targetDocument, Reference itemReference)
  {
    itemReference.frameIndex = -1;
    targetDocument.items.references = {itemReference};
  }

  void TimelineContext::frame_selection_set_for(Document& targetDocument, Reference frameReference)
  {
    group_selection_reset_for(targetDocument);
    targetDocument.editTarget = Document::EditTarget::FRAME;
    targetDocument.frame_references_set({frameReference});
    frames_selection_sync_for(targetDocument);
  }

  void TimelineContext::frame_selection_toggle_for(Document& targetDocument, Reference frameReference)
  {
    group_selection_reset_for(targetDocument);
    targetDocument.editTarget = Document::EditTarget::FRAME;
    auto selection = targetDocument.frame_references_get(Document::FrameReferenceFallback::NONE);
    auto itemReference = item_reference_from_frame_get(frameReference);
    if (selection.contains(frameReference))
    {
      if (selection.size() > 1) selection.erase(frameReference);
      bool isItemStillSelected{};
      for (const auto& selectedFrame : selection)
        if (is_same_item(selectedFrame, itemReference)) isItemStillSelected = true;
      if (!isItemStillSelected && targetDocument.items.references.size() > 1)
        targetDocument.items.references.erase(itemReference);
    }
    else
    {
      selection.insert(frameReference);
      targetDocument.items.references.insert(itemReference);
    }
    targetDocument.reference = frameReference;
    targetDocument.frame_references_set(std::move(selection));
    frames_selection_sync_for(targetDocument);
  }

  bool TimelineContext::frame_selection_range_set_for(Document& targetDocument, Reference firstReference,
                                                      Reference lastReference, bool isAdditive)
  {
    group_selection_reset_for(targetDocument);
    targetDocument.editTarget = Document::EditTarget::FRAME;
    if (!is_same_item(firstReference, lastReference) || firstReference.frameIndex < 0 || lastReference.frameIndex < 0)
      return false;

    auto item = command_item_reference_get(targetDocument, lastReference);
    if (!item) return false;
    if (firstReference.frameIndex >= (int)item->children.size() ||
        lastReference.frameIndex >= (int)item->children.size())
      return false;

    auto firstIndex = firstReference.frameIndex;
    auto lastIndex = lastReference.frameIndex;
    if (firstIndex > lastIndex) std::swap(firstIndex, lastIndex);
    auto selectedFrames = isAdditive ? targetDocument.frame_references_get(Document::FrameReferenceFallback::NONE)
                                     : std::set<Reference>{};

    for (int i = firstIndex; i <= lastIndex; ++i)
    {
      auto frameReference = lastReference;
      frameReference.frameIndex = i;
      selectedFrames.insert(frameReference);
    }

    targetDocument.reference = lastReference;
    targetDocument.frame_references_set(std::move(selectedFrames));
    frames_selection_sync_for(targetDocument);
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

    auto selectedItems = document.items.references;
    if (selectedItems.empty() && reference.itemType != NONE && reference.frameIndex < 0)
    {
      auto itemReference = reference;
      itemReference.frameIndex = -1;
      selectedItems.insert(itemReference);
    }

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
    frameSelectionSnapshot.clear();
    frameSelectionLocked.clear();
    isFrameSelectionLocked = false;
    frameFocusRequested = false;
    frameFocusIndex = -1;
    frameSelectionSnapshotReference = targetDocument.reference;
  }

  void TimelineContext::frames_selection_set_reference_for(Document& targetDocument)
  {
    auto& targetReference = targetDocument.reference;
    if (targetReference.frameIndex >= 0)
    {
      targetDocument.editTarget = Document::EditTarget::FRAME;
      targetDocument.frame_references_set({targetReference});
    }
    else
      targetDocument.frame_references_clear();
    frameSelectionSnapshot.assign(targetDocument.frames.selection.begin(), targetDocument.frames.selection.end());
    frameSelectionSnapshotReference = targetReference;
    frameSelectionLocked.clear();
    isFrameSelectionLocked = false;
    frameFocusIndex = targetReference.frameIndex;
    frameFocusRequested = targetReference.frameIndex >= 0;
  }

  void TimelineContext::frames_reference_normalize_for(Document& targetDocument)
  {
    auto selectedFrames = targetDocument.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (!selectedFrames.empty())
    {
      targetDocument.frame_references_set(std::move(selectedFrames));
      frames_selection_sync_for(targetDocument);
      return;
    }

    if (targetDocument.reference.frameIndex >= 0 && !targetDocument.is_frame_reference_valid(targetDocument.reference))
      targetDocument.reference.frameIndex = -1;
  }

  void TimelineContext::reference_clear_for(Document& targetDocument)
  {
    targetDocument.reference = {targetDocument.reference.animationIndex};
    frames_selection_reset_for(targetDocument);
    targetDocument.items.references.clear();
  }

  void TimelineContext::reference_set_item_reference_for(Document& targetDocument, Reference itemReference)
  {
    itemReference.frameIndex = -1;
    targetDocument.reference = itemReference;
    frames_selection_reset_for(targetDocument);
    item_selection_set_for(targetDocument, itemReference);
  }

  void TimelineContext::reference_set_timeline_item_reference_for(Document& targetDocument, Reference itemReference)
  {
    if (itemReference.itemType == LAYER)
      if (auto layer = targetDocument.anm2.element_get(ElementType::LAYER_ELEMENT, itemReference.itemID))
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

    auto track_row_push = [&](const Element& item, int type, int index, int depth = 0)
    {
      auto rootGroupType = item.groupId == -1 ? NONE : type;
      rows.push_back({.type = type,
                      .id = anm2ed::track_id_get(item),
                      .index = index,
                      .groupId = item.groupId,
                      .rootGroupType = rootGroupType,
                      .rootGroupId = item.groupId,
                      .depth = depth});
    };
    auto group_row_push = [&](const Element& group, int type, int index)
    { rows.push_back({.type = type, .id = group.id, .index = index, .isGroup = true}); };
    auto group_root_row_push = [&](const Element& group, int type)
    { rows.push_back({.type = ROOT, .id = -1, .rootGroupType = type, .rootGroupId = group.id, .depth = 1}); };
    auto group_ids_get = [](const Element& container)
    {
      std::set<int> result{};
      for (const auto& item : container.children)
        if (item.type == ElementType::GROUP) result.insert(item.id);
      return result;
    };

    rows.push_back({.type = ROOT});

    for (const auto& containerRow : TRACK_CONTAINERS)
    {
      auto container = child_first_get(*animation, containerRow.container);
      if (!container) continue;
      auto type = (int)containerRow.itemType;
      auto isReversed = containerRow.itemType == ItemType::LAYER;
      auto count = (int)container->children.size();
      auto index_get = [&](int i) { return isReversed ? count - 1 - i : i; };
      auto groupIds = group_ids_get(*container);
      auto is_track_shown = [&](const Element& item)
      { return item.type == containerRow.track && (settings.timelineIsShowUnused || !item.children.empty()); };

      for (int i = 0; i < count; ++i)
      {
        auto index = index_get(i);
        auto& item = container->children[index];
        if (item.type == ElementType::GROUP)
        {
          group_row_push(item, type, index);
          if (!item.isExpanded) continue;
          group_root_row_push(item, type);
          for (int j = 0; j < count; ++j)
            if (auto& child = container->children[index_get(j)]; is_track_shown(child) && child.groupId == item.id)
              track_row_push(child, type, index_get(j), 1);
        }
        else if (is_track_shown(item) && !groupIds.contains(item.groupId))
          track_row_push(item, type, index);
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
    return document.groupReferences.contains(group_reference_get(row));
  }

  bool TimelineContext::is_row_selected(const TimelineItemRow& row)
  {
    if (row.isGroup) return is_group_selected(row);
    auto itemReference = item_reference_get(row.type, row.id, row.rootGroupType, row.rootGroupId);
    auto isReferenced = is_same_item(reference, itemReference);
    return document.items.references.contains(itemReference) ||
           (document.items.references.empty() && document.groupReferences.empty() && isReferenced);
  }

  void TimelineContext::row_selection_clear()
  {
    document.items.references.clear();
    document.groupReferences.clear();
  }

  void TimelineContext::row_selection_insert(const TimelineRowReference& row)
  {
    if (row.isGroup)
      document.groupReferences.insert({row.animationIndex, row.type, row.id});
    else
      document.items.references.insert(row_item_reference_get(row));
  }

  void TimelineContext::row_selection_erase(const TimelineRowReference& row)
  {
    if (row.isGroup)
      document.groupReferences.erase({row.animationIndex, row.type, row.id});
    else
      document.items.references.erase(row_item_reference_get(row));
  }

  bool TimelineContext::is_row_reference_selected(const TimelineRowReference& row)
  {
    if (row.isGroup) return document.groupReferences.contains({row.animationIndex, row.type, row.id});
    return document.items.references.contains(row_item_reference_get(row));
  }

  std::size_t TimelineContext::row_selection_count_get()
  {
    return document.items.references.size() + document.groupReferences.size();
  }

  void TimelineContext::row_selection_set(const TimelineItemRow& row)
  {
    auto rowReference = row_reference_get(row);
    auto isCtrlDown = ImGui::IsKeyDown(ImGuiMod_Ctrl);
    auto isShiftDown = ImGui::IsKeyDown(ImGuiMod_Shift);

    if (row.isGroup)
      reference = {reference.animationIndex};
    else
    {
      if (row.type == LAYER)
        if (auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, row.id))
          document.spritesheet.reference = layer->spritesheetId;
      reference = row_item_reference_get(rowReference);
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
