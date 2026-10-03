#include "context.hpp"

namespace anm2ed::imgui
{
  void TimelineContext::frame_insert()
  {
    auto targetReference = reference;
    auto time = (int)document.frameTime;
    if (!animation || !command_item_reference_get(document, targetReference)) return;
    edit_push(EDIT_INSERT_FRAME, [=](Anm2& anm2) { return edit::frame_insert(anm2, targetReference, time); });
  }

  void TimelineContext::frames_delete_action()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (selectedFrames.empty()) return;
    edit_push(
        EDIT_DELETE_FRAMES, [=](Anm2& anm2) { return edit::frames_delete(anm2, selectedFrames); },
        [this](Document& document, const edit::Uids&) { frames_selection_set_reference_for(document); });
  }

  void TimelineContext::frames_duplicate()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    std::erase_if(selectedFrames, is_trigger_reference);
    if (selectedFrames.empty()) return;
    edit_push(EDIT_DUPLICATE_FRAMES,
              [=, focus = reference](Anm2& anm2) { return edit::frames_duplicate(anm2, selectedFrames, focus); });
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
    edit_push(EDIT_REVERSE_FRAMES, [=](Anm2& anm2) { return edit::frames_reverse(anm2, selectedFrames); });
  }

  void TimelineContext::frames_bake()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    std::erase_if(selectedFrames, is_trigger_reference);
    if (selectedFrames.empty()) return;
    edit_push(EDIT_BAKE_FRAMES, [=, interval = settings.bakeInterval, isRoundScale = settings.bakeIsRoundScale,
                                 isRoundRotation = settings.bakeIsRoundRotation](Anm2& anm2)
              { return edit::frames_bake(anm2, selectedFrames, interval, isRoundScale, isRoundRotation); });
  }

  std::set<Reference> TimelineContext::selected_root_frame_references_get()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    std::erase_if(selectedFrames, [](const Reference& frameReference) { return frameReference.itemType != ROOT; });
    return selectedFrames;
  }

  std::set<Reference> TimelineContext::item_references_for_bake_into_other_frames_get(const Document& targetDocument,
                                                                                      const Element& targetAnimation,
                                                                                      BakeIntoOtherFramesTarget target,
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

    for (const auto& row : TRACK_CONTAINERS)
    {
      auto container = child_first_get(targetAnimation, row.container);
      if (!container || !(row.itemType == ItemType::LAYER ? isLayers : isNulls)) continue;
      tracks_each(*container, row.track,
                  [&](const Element& track)
                  {
                    auto itemType = (int)row.itemType;
                    result.insert(Reference{animationIndex, itemType, anm2ed::track_id_get(track), -1,
                                            track.groupId == -1 ? NONE : itemType, track.groupId});
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
    edit_push(EDIT_BAKE_INTO_OTHER_FRAMES,
              [=](Anm2& anm2) { return edit::root_bake_into(anm2, selectedRootFrames, targetItems, options); });
  }

  void TimelineContext::frame_split()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (selectedFrames.size() != 1) return;
    auto targetReference = *selectedFrames.begin();
    auto time = frameSplitTimeAtCursor.value_or((int)std::floor(playback.time));
    edit_push(EDIT_SPLIT_FRAME, [=](Anm2& anm2) { return edit::frame_split(anm2, targetReference, time); });
  }

  void TimelineContext::item_remove()
  {
    std::map<int, std::set<int>> ids{};
    std::map<int, std::set<int>> groupIds{};
    for (auto row : selected_row_references_get())
      if (row.type == LAYER || row.type == NULL_) (row.isGroup ? groupIds : ids)[row.type].insert(row.id);
    if (!animation || (ids.empty() && groupIds.empty())) return;
    edit_push(
        EDIT_REMOVE_ITEMS, [=, animationIndex = reference.animationIndex](Anm2& anm2)
        { return edit::items_remove(anm2, animationIndex, ids, groupIds); },
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
         name = std::string(localize.get(TEXT_NEW_GROUP))](Anm2& anm2)
        { return edit::items_group(anm2, animationIndex, type, ids, name); },
        [=, this](Document& document, const edit::Uids& uids)
        {
          if (uids.empty()) return;
          document.selected_set(SelectionKind::TRACKS, {targetReferences.begin(), targetReferences.end()});
          document.reference_set(targetReferences.front());
          frames_selection_reset_for(document);
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
    selection_references_set(moved, anm2, SelectionKind::TRACKS, tracks);
    selection_references_set(moved, anm2, SelectionKind::GROUPS, groups);
    if (!tracks.empty()) selection_focus_set(moved, anm2, *tracks.begin());

    edit_push(
        EDIT_MOVE_ITEMS,
        [=](Anm2& anm2)
        {
          return edit::items_move(anm2, animationIndex, targetType, ids, groupIds, target, isDropAfter,
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
          frames_selection_reset_for(document);
        });
  }

  void TimelineContext::fit_animation_length()
  {
    if (!animation) return;
    edit_push(EDIT_FIT_ANIMATION_LENGTH, [animationIndex = reference.animationIndex](Anm2& anm2)
              { return edit::animation_length_fit(anm2, animationIndex); });
  }

  void TimelineContext::frame_references_copy(const std::set<Reference>& selectedFrames)
  {
    if (!animation || selectedFrames.empty()) return;

    std::string clipboardString{};
    for (auto frameReference : selectedFrames)
    {
      auto item =
          item_get(frameReference.itemType, frameReference.itemID, frameReference.groupType, frameReference.groupId);
      auto frame = item ? track_frame_get(*item, frameReference.frameIndex) : nullptr;
      if (!frame) continue;
      auto parentType = TYPE_TRACKS[frameReference.itemType];
      clipboardString += element_to_string(*frame, parentType);
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
        EDIT_CUT_FRAMES, [=](Anm2& anm2) { return edit::frames_delete(anm2, selectedFrames); },
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
         time = hoveredTime](Anm2& anm2)
        { return edit::frames_paste(anm2, targetReference, selectedFrames, text, time, errorString.get()); },
        [=, this](Document& document, const edit::Uids& uids)
        {
          if (errorString->empty()) return frames_select_for(document, uids);
          toasts.push(std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED), *errorString));
          logger.error(
              std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED, anm2ed::ENGLISH), *errorString));
        });
  }
}
