#include "context.hpp"

namespace anm2ed::imgui
{
  void TimelineContext::frame_insert()
  {
    auto targetReference = reference;
    auto time = (int)document.frameTime;
    if (!animation || !command_item_reference_get(document, targetReference)) return;
    edit_push(EDIT_INSERT_FRAME, [=](model::Model& model) { return edit::frame_insert(model, targetReference, time); });
  }

  void TimelineContext::frames_delete_action()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (selectedFrames.empty()) return;
    edit_push(
        EDIT_DELETE_FRAMES, [=](model::Model& model) { return edit::frames_delete(model, selectedFrames); },
        [this](Document& document, const edit::Uids&) { frames_selection_set_reference_for(document); });
  }

  void TimelineContext::frames_duplicate()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    std::erase_if(selectedFrames, is_trigger_reference);
    if (selectedFrames.empty()) return;
    edit_push(EDIT_DUPLICATE_FRAMES, [=, focus = reference](model::Model& model)
              { return edit::frames_duplicate(model, selectedFrames, focus); });
  }

  bool TimelineContext::is_frames_reverse_available(std::set<Reference> selectedFrames)
  {
    std::map<Reference, int> counts{};
    std::erase_if(selectedFrames, is_trigger_reference);
    for (auto frameReference : selectedFrames)
    {
      auto itemReference = item_reference_from_frame_get(frameReference);
      ++counts[itemReference];
    }
    return std::ranges::any_of(counts, [](const auto& item) { return item.second >= 2; });
  }

  void TimelineContext::frames_reverse()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    std::erase_if(selectedFrames, is_trigger_reference);
    if (!is_frames_reverse_available(selectedFrames)) return;
    edit_push(EDIT_REVERSE_FRAMES, [=](model::Model& model) { return edit::frames_reverse(model, selectedFrames); });
  }

  void TimelineContext::frames_bake()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    std::erase_if(selectedFrames, is_trigger_reference);
    if (selectedFrames.empty()) return;
    edit_push(EDIT_BAKE_FRAMES, [=, interval = settings.bakeInterval, isRoundScale = settings.bakeIsRoundScale,
                                 isRoundRotation = settings.bakeIsRoundRotation](model::Model& model)
              { return edit::frames_bake(model, selectedFrames, interval, isRoundScale, isRoundRotation); });
  }

  std::set<Reference> TimelineContext::selected_root_frame_references_get()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    std::erase_if(selectedFrames, [](const Reference& frameReference) { return frameReference.itemType != ROOT; });
    return selectedFrames;
  }

  std::set<Reference> TimelineContext::item_references_for_bake_into_other_frames_get(
      const Document& targetDocument, const model::Animation& targetAnimation, BakeIntoOtherFramesTarget target,
      bool isLayers, bool isNulls)
  {
    std::set<Reference> result{};
    auto animationIndex = targetDocument.reference_get().animationIndex;
    if (target == BakeIntoOtherFramesTarget::CURRENT_SELECTION)
    {
      for (auto itemReference : targetDocument.selected_get(SelectionKind::TRACKS))
        if (itemReference.itemType == LAYER || itemReference.itemType == NULL_) result.insert(itemReference);
      return result;
    }

    for (auto itemType : {LAYER, NULL_})
    {
      if (!(itemType == LAYER ? isLayers : isNulls)) continue;
      model::tracks_each(model::animation_entries_get(targetAnimation, itemType),
                         [&](const model::Track& track, const model::TrackGroup* group)
                         {
                           result.insert(Reference{animationIndex, itemType, track.id, -1, group ? itemType : NONE,
                                                   group ? group->id : -1});
                         });
    }
    return result;
  }

  bool TimelineContext::is_bake_into_other_frames_ready()
  {
    auto selectedRootFrames = selected_root_frame_references_get();
    if (selectedRootFrames.empty() || !animation) return false;
    auto targetItems = item_references_for_bake_into_other_frames_get(
        document, *animation, bakeIntoOtherFramesTarget, isBakeIntoOtherFramesLayers, isBakeIntoOtherFramesNulls);
    return !targetItems.empty();
  }

  void TimelineContext::bake_into_other_frames()
  {
    auto selectedRootFrames = selected_root_frame_references_get();
    if (selectedRootFrames.empty() || !animation) return;
    auto targetItems = item_references_for_bake_into_other_frames_get(
        document, *animation, bakeIntoOtherFramesTarget, isBakeIntoOtherFramesLayers, isBakeIntoOtherFramesNulls);
    edit::RootBakeOptions options{settings.bakeIsRoundScale, settings.bakeIsRoundRotation,
                                  settings.bakeIsMatchRootInterpolation, settings.bakeIsUseRootPivot};
    edit_push(EDIT_BAKE_INTO_OTHER_FRAMES, [=](model::Model& model)
              { return edit::root_bake_into(model, selectedRootFrames, targetItems, options); });
  }

  void TimelineContext::frame_split()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (selectedFrames.size() != 1) return;
    auto targetReference = *selectedFrames.begin();
    auto time = frameSplitTimeAtCursor.value_or((int)std::floor(playback.time));
    edit_push(EDIT_SPLIT_FRAME, [=](model::Model& model) { return edit::frame_split(model, targetReference, time); });
  }

  void TimelineContext::item_remove()
  {
    std::map<int, std::set<int>> ids{};
    std::map<int, std::set<int>> groupIds{};
    for (auto row : selected_row_references_get())
      if (row.type == LAYER || row.type == NULL_) (row.isGroup ? groupIds : ids)[row.type].insert(row.id);
    if (!animation || (ids.empty() && groupIds.empty())) return;
    edit_push(
        EDIT_REMOVE_ITEMS, [=, animationIndex = reference.animationIndex](model::Model& model)
        { return edit::items_remove(model, animationIndex, ids, groupIds); },
        [this](Document& document, const edit::Uids&)
        {
          reference_clear_for(document);
          group_selection_reset_for(document);
        });
  }

  std::vector<Reference> TimelineContext::item_references_groupable_get()
  {
    if (!document.selection.uids[SelectionKind::GROUPS].empty()) return std::vector<Reference>{};
    auto selectedItems = item_references_for_current_get();
    std::erase_if(selectedItems, [](const Reference& itemReference)
                  { return itemReference.itemType != LAYER && itemReference.itemType != NULL_; });
    if (selectedItems.empty()) return std::vector<Reference>{};
    auto itemType = selectedItems.begin()->itemType;
    for (const auto& itemReference : selectedItems)
      if (itemReference.itemType != itemType) return std::vector<Reference>{};

    std::vector<Reference> result{};
    for (const auto& row : timeline_item_rows_get())
    {
      if (row.isGroup || row.type != itemType) continue;
      auto itemReference = item_reference_get(row.type, row.id, NONE, -1);
      if (!selectedItems.contains(itemReference)) continue;
      if (row.groupId != -1) return std::vector<Reference>{};
      result.push_back(itemReference);
    }
    return result;
  }

  void TimelineContext::item_group()
  {
    auto targetReferences = item_references_groupable_get();
    if (targetReferences.empty()) return;
    std::set<int> ids{};
    for (auto itemReference : targetReferences)
      ids.insert(itemReference.itemID);
    edit_push(
        EDIT_GROUP_ITEMS,
        [=, animationIndex = reference.animationIndex, type = targetReferences.front().itemType,
         name = std::string(localize.get(TEXT_NEW_GROUP))](model::Model& model)
        { return edit::items_group(model, animationIndex, type, ids, name); },
        [=, this](Document& document, const edit::Uids& uids)
        {
          if (uids.empty()) return;
          document.selected_set(SelectionKind::TRACKS, {targetReferences.begin(), targetReferences.end()});
          document.reference_set(targetReferences.front());
          document.frame_references_clear();
        });
  }

  void TimelineContext::rows_move_to_row(std::vector<TimelineRowReference> draggedRows, TimelineItemRow targetRow,
                                         bool isDropAfter, bool isDropIntoGroup)
  {
    if (draggedRows.empty()) return;
    auto targetType = draggedRows.front().type;
    if (targetType != LAYER && targetType != NULL_) return;
    if (std::ranges::any_of(draggedRows, [&](const auto& row) { return row.type != targetType; })) return;
    if (targetRow.isGroup && targetRow.type != targetType) return;
    if (!targetRow.isGroup && targetRow.type != targetType && targetRow.type != NONE && targetRow.type != ROOT &&
        targetRow.type != TRIGGER)
      return;

    std::set<int> ids{};
    std::set<int> groupIds{};
    for (const auto& row : draggedRows)
      (row.isGroup ? groupIds : ids).insert(row.id);
    edit::RowTarget target{targetRow.isGroup, targetRow.type, targetRow.id, targetRow.groupId};
    auto animationIndex = reference.animationIndex;
    std::set<Reference> tracks{};
    std::set<Reference> groups{};
    for (const auto& row : draggedRows)
      (row.isGroup ? groups : tracks)
          .insert(row.isGroup ? Reference{row.animationIndex, row.type, row.id} : row_item_reference_get(row));
    Selection moved{};
    selection_references_set(moved, model, SelectionKind::TRACKS, tracks);
    selection_references_set(moved, model, SelectionKind::GROUPS, groups);
    if (!tracks.empty()) selection_focus_set(moved, model, *tracks.begin());

    edit_push(
        EDIT_MOVE_ITEMS,
        [=](model::Model& model)
        {
          return edit::items_move(model, animationIndex, targetType, ids, groupIds, target, isDropAfter,
                                  isDropIntoGroup);
        },
        [=, this](Document& document, const edit::Uids&)
        {
          document.selection.uids[SelectionKind::TRACKS] = moved.uids.at(SelectionKind::TRACKS);
          document.selection.uids[SelectionKind::GROUPS] = moved.uids.at(SelectionKind::GROUPS);
          if (tracks.empty())
            document.reference_set({document.reference_get().animationIndex});
          else
            document.selection.focus = moved.focus;
          document.frame_references_clear();
        });
  }

  void TimelineContext::fit_animation_length()
  {
    if (!animation) return;
    edit_push(EDIT_FIT_ANIMATION_LENGTH, [animationIndex = reference.animationIndex](model::Model& model)
              { return edit::animation_length_fit(model, animationIndex); });
  }

  void TimelineContext::frame_references_copy(const std::set<Reference>& selectedFrames)
  {
    if (!animation || selectedFrames.empty()) return;

    std::string clipboardString{};
    for (auto frameReference : selectedFrames)
    {
      auto frame = model.frame_get(frameReference);
      if (!frame) continue;
      clipboardString += model::frame_to_string(*frame, (ItemType)frameReference.itemType);
    }
    if (!clipboardString.empty()) clipboard.set(clipboardString);
  }

  void TimelineContext::copy()
  {
    auto selectedFrames = copy_frame_references_get();
    if (selectedFrames.empty()) return;

    frame_references_copy(selectedFrames);
  }

  void TimelineContext::cut()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (selectedFrames.empty()) return;
    frame_references_copy(selectedFrames);
    edit_push(
        EDIT_CUT_FRAMES, [=](model::Model& model) { return edit::frames_delete(model, selectedFrames); },
        [this](Document& document, const edit::Uids&) { frames_selection_set_reference_for(document); });
  }

  void TimelineContext::paste()
  {
    auto text = clipboard.get();
    if (text.empty()) return;
    if (!command_item_reference_get(document, reference))
    {
      toast_log(Level::WARNING, TOAST_DESERIALIZE_FRAMES_NO_SELECTION);
      return;
    }

    auto errorString = std::make_shared<std::string>();
    edit_push(
        EDIT_PASTE_FRAMES,
        [=, targetReference = reference,
         selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE),
         time = hoveredTime](model::Model& model)
        { return edit::frames_paste(model, targetReference, selectedFrames, text, time, errorString.get()); },
        [=, this](Document& document, const edit::Uids& uids)
        {
          if (errorString->empty()) return frames_select_for(document, uids);
          toasts.push(std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED), *errorString));
          logger.error(
              std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED, anm2ed::ENGLISH), *errorString));
        });
  }
}
