#include "context.hpp"

namespace anm2ed::imgui
{
  void TimelineContext::frame_insert()
  {
    auto targetReference = reference;
    auto targetFrameTime = document.frameTime;
    if (!animation || !command_item_reference_get(document, targetReference)) return;

    edit_command_push(EDIT_INSERT_FRAME, Document::FRAMES,
                      [=, this](Manager&, Document& document) mutable
                      {
                        auto animation =
                            document.anm2.element_get(ElementType::ANIMATION, targetReference.animationIndex);
                        auto item = command_item_reference_get(document, targetReference);
                        if (!animation || !item) return;

                        auto newReference = targetReference;

                        if (targetReference.itemType == TRIGGER)
                        {
                          for (auto& trigger : item->children)
                            if (targetFrameTime == trigger.atFrame) return;

                          auto addFrame = element_make(ElementType::TRIGGER);
                          addFrame.atFrame = targetFrameTime;
                          item->children.push_back(addFrame);
                          frames_sort_by_at_frame(*item);
                          newReference.frameIndex = frame_index_from_at_frame_get(*item, addFrame.atFrame);
                        }
                        else
                        {
                          auto frame = command_frame_get(document, targetReference);
                          auto framesCount = track_frames_count_get(*item);
                          if (frame)
                          {
                            auto addFrame = *frame;
                            auto insertIndex = std::clamp(targetReference.frameIndex + 1, 0, framesCount);
                            auto childIndex = track_frame_insert_child_index_get(*item, insertIndex);
                            item->children.insert(item->children.begin() + childIndex, addFrame);
                            newReference.frameIndex = insertIndex;
                          }
                          else if (auto lastFrame = track_frame_get(*item, framesCount - 1))
                          {
                            item->children.emplace_back(*lastFrame);
                            newReference.frameIndex = framesCount;
                          }
                          else
                          {
                            item->children.emplace_back(element_make(ElementType::FRAME));
                            newReference.frameIndex = 0;
                          }
                        }

                        document.reference = newReference;
                        frames_selection_set_reference_for(document);
                        if (newReference.itemType != TRIGGER)
                          document.frameTime = frame_time_from_index_get(*item, newReference.frameIndex);
                      },
                      SnapshotKind::TRACKS, {targetReference});
  }

  void TimelineContext::frames_delete_for(Document& document, std::set<Reference> selectedFrames)
  {
    auto groupedFrames = frames_by_item_get(selectedFrames);

    for (auto& [itemReference, indices] : groupedFrames)
    {
      auto item = command_item_reference_get(document, itemReference);
      if (!item) continue;

      for (auto it = indices.rbegin(); it != indices.rend(); ++it)
      {
        auto childIndex = track_frame_child_index_get(*item, *it);
        if (childIndex != -1) item->children.erase(item->children.begin() + childIndex);
      }
    }

    auto item = command_item_reference_get(document, document.reference);
    if (!item || item->children.empty())
    {
      document.reference.frameIndex = -1;
      frames_selection_reset_for(document);
      return;
    }

    document.reference.frameIndex = glm::clamp(document.reference.frameIndex, 0, (int)item->children.size() - 1);
    frames_selection_set_reference_for(document);
  }

  void TimelineContext::frames_delete_action()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (selectedFrames.empty()) return;
    auto trackReferences = track_references_from_frame_references_get(selectedFrames);
    edit_command_push(
        EDIT_DELETE_FRAMES, Document::FRAMES, [=, this](Manager&, Document& document) mutable
        { frames_delete_for(document, selectedFrames); }, SnapshotKind::TRACKS, trackReferences);
  }

  void TimelineContext::frames_duplicate()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    std::erase_if(selectedFrames, is_trigger_reference);
    if (selectedFrames.empty()) return;

    auto trackReferences = track_references_from_frame_references_get(selectedFrames);
    edit_command_push(
        EDIT_DUPLICATE_FRAMES, Document::FRAMES,
        [=, this](Manager&, Document& document) mutable
        {
          auto groupedFrames = frames_by_item_get(selectedFrames);

          std::set<Reference> duplicatedSelection{};
          auto newReference = document.reference;
          bool isReferenceSet{};

          for (auto& [itemReference, indices] : groupedFrames)
          {
            auto item = command_item_reference_get(document, itemReference);
            if (!item) continue;

            std::vector<int> validIndices{};
            std::vector<Element> duplicatedFrames{};
            for (auto i : indices)
            {
              auto frame = track_frame_get(*item, i);
              if (!frame) continue;
              validIndices.push_back(i);
              duplicatedFrames.push_back(*frame);
            }
            if (validIndices.empty()) continue;

            auto insertIndex = validIndices.back() + 1;
            auto childIndex = track_frame_insert_child_index_get(*item, insertIndex);
            item->children.insert(item->children.begin() + childIndex,
                                  std::make_move_iterator(duplicatedFrames.begin()),
                                  std::make_move_iterator(duplicatedFrames.end()));

            for (int offset = 0; offset < (int)validIndices.size(); ++offset)
            {
              auto targetReference = itemReference;
              targetReference.frameIndex = insertIndex + offset;
              duplicatedSelection.insert(targetReference);
              auto sourceReference = itemReference;
              sourceReference.frameIndex = validIndices[offset];
              if (sourceReference == document.reference)
              {
                newReference = targetReference;
                isReferenceSet = true;
              }
            }
          }

          if (duplicatedSelection.empty()) return;
          document.reference = isReferenceSet ? newReference : *duplicatedSelection.begin();
          document.frame_references_set(std::move(duplicatedSelection));
          if (auto item = command_item_reference_get(document, document.reference))
            document.frameTime = frame_time_from_index_get(*item, document.reference.frameIndex);
          frames_focus_sync_for(document);
        },
        SnapshotKind::TRACKS, trackReferences);
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

    auto trackReferences = track_references_from_frame_references_get(selectedFrames);
    edit_command_push(
        EDIT_REVERSE_FRAMES, Document::FRAMES,
        [=, this](Manager&, Document& document) mutable
        {
          auto groupedFrames = frames_by_item_get(selectedFrames);

          std::set<Reference> reversedSelection{};
          auto newReference = document.reference;
          bool isReferenceSet{};

          for (auto& [itemReference, indices] : groupedFrames)
          {
            auto item = command_item_reference_get(document, itemReference);
            if (!item || indices.size() < 2) continue;

            std::vector<int> validIndices{};
            std::vector<Element> reversedFrames{};
            for (auto i : indices)
            {
              auto frame = track_frame_get(*item, i);
              if (!frame) continue;
              validIndices.push_back(i);
              reversedFrames.push_back(std::move(*frame));
            }
            if (validIndices.size() < 2) continue;

            auto insertIndex = validIndices.front();
            for (auto it = validIndices.rbegin(); it != validIndices.rend(); ++it)
            {
              auto childIndex = track_frame_child_index_get(*item, *it);
              if (childIndex != -1) item->children.erase(item->children.begin() + childIndex);
            }

            std::ranges::reverse(reversedFrames);
            auto childIndex = track_frame_insert_child_index_get(*item, insertIndex);
            item->children.insert(item->children.begin() + childIndex, std::make_move_iterator(reversedFrames.begin()),
                                  std::make_move_iterator(reversedFrames.end()));

            for (int offset = 0; offset < (int)validIndices.size(); ++offset)
            {
              auto targetReference = itemReference;
              targetReference.frameIndex = insertIndex + offset;
              reversedSelection.insert(targetReference);
              auto sourceIndex = validIndices[(int)validIndices.size() - 1 - offset];
              auto sourceReference = itemReference;
              sourceReference.frameIndex = sourceIndex;
              if (sourceReference == document.reference)
              {
                newReference = targetReference;
                isReferenceSet = true;
              }
            }
          }

          if (reversedSelection.empty()) return;
          document.reference = isReferenceSet ? newReference : *reversedSelection.begin();
          document.frame_references_set(std::move(reversedSelection));
          if (auto item = command_item_reference_get(document, document.reference))
            document.frameTime = frame_time_from_index_get(*item, document.reference.frameIndex);
          frames_focus_sync_for(document);
        },
        SnapshotKind::TRACKS, trackReferences);
  }

  void TimelineContext::frames_bake()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    auto bakeInterval = settings.bakeInterval;
    auto isRoundScale = settings.bakeIsRoundScale;
    auto isRoundRotation = settings.bakeIsRoundRotation;
    std::erase_if(selectedFrames, is_trigger_reference);
    if (selectedFrames.empty()) return;

    auto trackReferences = track_references_from_frame_references_get(selectedFrames);
    edit_command_push(
        EDIT_BAKE_FRAMES, Document::FRAMES,
        [=, this](Manager&, Document& document) mutable
        {
          auto groupedFrames = frames_by_item_get(selectedFrames);

          std::set<Reference> bakedSelection{};
          for (auto& [itemReference, indices] : groupedFrames)
          {
            auto item = command_item_reference_get(document, itemReference);
            if (!item) continue;

            int insertedBefore = 0;
            for (auto originalIndex : indices)
            {
              auto i = originalIndex + insertedBefore;
              auto frame = track_frame_get(*item, i);
              if (!frame) continue;

              auto originalDuration = frame->duration;
              frame_bake(*item, i, bakeInterval, isRoundScale, isRoundRotation);

              auto bakedCount =
                  originalDuration <= FRAME_DURATION_MIN ? 1 : (int)std::ceil((float)originalDuration / bakeInterval);
              for (int offset = 0; offset < bakedCount; ++offset)
              {
                auto frameReference = itemReference;
                frameReference.frameIndex = i + offset;
                bakedSelection.insert(frameReference);
              }

              insertedBefore += bakedCount - 1;
            }
          }

          document.frame_references_set(std::move(bakedSelection));
          if (!document.frames.references.empty())
          {
            frames_focus_sync_for(document);
          }
          else
            frames_selection_reset_for(document);
        },
        SnapshotKind::TRACKS, trackReferences);
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
    auto animationIndex = targetDocument.reference.animationIndex;
    if (target == BakeIntoOtherFramesTarget::CURRENT_SELECTION)
    {
      for (auto itemReference : targetDocument.items.references)
      {
        itemReference.frameIndex = -1;
        if (itemReference.itemType == LAYER || itemReference.itemType == NULL_) result.insert(itemReference);
      }
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
                    result.insert({animationIndex, itemType, anm2ed::track_id_get(track), -1,
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

  void TimelineContext::frame_root_transform_apply(Element& frame, const Element& rootFrame, bool isRoundScale,
                                                   bool isRoundRotation, bool isUseRootPivot)
  {
    auto rootScale = math::percent_to_unit(rootFrame.scale);
    auto pivot = isUseRootPivot ? rootFrame.position : glm::vec2();
    auto offset = (frame.position - pivot) * rootScale;
    auto radians = glm::radians(rootFrame.rotation);
    auto cos = std::cos(radians);
    auto sin = std::sin(radians);

    frame.position = rootFrame.position + glm::vec2(offset.x * cos - offset.y * sin, offset.x * sin + offset.y * cos);
    frame.scale *= rootScale;
    frame.rotation += rootFrame.rotation;
    frame.tint *= rootFrame.tint;
    frame.colorOffset += rootFrame.colorOffset;

    if (isRoundScale) frame.scale = glm::round(frame.scale);
    if (isRoundRotation) frame.rotation = std::round(frame.rotation);
  }

  void TimelineContext::bake_into_other_frames()
  {
    auto selectedRootFrames = selected_root_frame_references_get();
    if (selectedRootFrames.empty()) return;
    if (!animation) return;
    auto target = bakeIntoOtherFramesTarget;
    auto isLayers = isBakeIntoOtherFramesLayers;
    auto isNulls = isBakeIntoOtherFramesNulls;
    auto isRoundScale = settings.bakeIsRoundScale;
    auto isRoundRotation = settings.bakeIsRoundRotation;
    auto isMatchRootInterpolation = settings.bakeIsMatchRootInterpolation;
    auto isUseRootPivot = settings.bakeIsUseRootPivot;
    auto targetItems = item_references_for_bake_into_other_frames_get(
        document, *animation, target, isBakeIntoOtherFramesLayers, isBakeIntoOtherFramesNulls);
    auto trackReferences = track_references_from_frame_references_get(selectedRootFrames);
    trackReferences.insert(targetItems.begin(), targetItems.end());
    edit_command_push(
        EDIT_BAKE_INTO_OTHER_FRAMES, Document::FRAMES,
        [=, this](Manager&, Document& document) mutable
        {
          auto animation = document.anm2.element_get(ElementType::ANIMATION, document.reference.animationIndex);
          if (!animation) return;
          auto rootItemReference = item_reference_from_frame_get(*selectedRootFrames.begin());
          for (auto rootReference : selectedRootFrames)
            if (!is_same_item(rootReference, rootItemReference)) return;
          auto root = command_item_reference_get(document, rootItemReference);
          if (!root) return;

          std::vector<RootFrameSpan> spans{};
          for (auto rootReference : selectedRootFrames)
          {
            auto rootFrame = command_frame_get(document, rootReference);
            if (!rootFrame) continue;
            auto start = (int)frame_time_from_index_get(*root, rootReference.frameIndex);
            spans.push_back({start, start + rootFrame->duration});
          }
          if (spans.empty()) return;
          auto is_spanned = [&](int time)
          {
            return std::ranges::any_of(spans,
                                       [&](RootFrameSpan span) { return time >= span.start && time < span.end; });
          };

          auto targetItems =
              item_references_for_bake_into_other_frames_get(document, *animation, target, isLayers, isNulls);
          if (targetItems.empty()) return;

          for (auto itemReference : targetItems)
          {
            auto item = command_item_reference_get(document, itemReference);
            if (!item) continue;

            if (isMatchRootInterpolation)
            {
              std::vector<int> bakeIndices{};
              auto frameType = track_frame_type_get(*item);
              int frameIndex{};
              for (const auto& frame : item->children)
              {
                if (frame.type != frameType) continue;
                if (is_spanned((int)frame_time_from_index_get(*item, frameIndex))) bakeIndices.push_back(frameIndex);
                ++frameIndex;
              }
              for (auto it = bakeIndices.rbegin(); it != bakeIndices.rend(); ++it)
                frame_bake(*item, *it, FRAME_DURATION_MIN, isRoundScale, isRoundRotation);
            }

            auto frameType = track_frame_type_get(*item);
            int frameIndex{};
            for (auto& frame : item->children)
            {
              if (frame.type != frameType) continue;
              auto frameStart = (int)frame_time_from_index_get(*item, frameIndex++);
              if (is_spanned(frameStart))
                frame_root_transform_apply(frame, frame_generate(*root, (float)frameStart), isRoundScale,
                                           isRoundRotation, isUseRootPivot);
            }
          }

          std::set<int> selectedRootFrameIndices{};
          for (auto rootReference : selectedRootFrames)
          {
            auto rootFrame = command_frame_get(document, rootReference);
            if (!rootFrame) continue;
            selectedRootFrameIndices.insert(rootReference.frameIndex);
          }
          if (selectedRootFrameIndices.empty()) return;

          std::vector<Element> rootFrames{};
          std::set<Reference> defaultRootSelection{};
          int frameIndex{};
          int rewrittenFrameIndex{};
          int defaultDuration{};
          auto default_root_frame_push = [&]()
          {
            if (defaultDuration <= 0) return;
            auto frame = element_make(ElementType::FRAME);
            frame.duration = glm::max(defaultDuration, FRAME_DURATION_MIN);
            auto defaultRootReference = rootItemReference;
            defaultRootReference.frameIndex = rewrittenFrameIndex;
            defaultRootSelection.insert(defaultRootReference);
            rootFrames.push_back(frame);
            ++rewrittenFrameIndex;
            defaultDuration = 0;
          };

          for (const auto& frame : root->children)
          {
            if (frame.type != ElementType::FRAME)
            {
              default_root_frame_push();
              rootFrames.push_back(frame);
              continue;
            }

            if (selectedRootFrameIndices.contains(frameIndex))
              defaultDuration += frame.duration;
            else
            {
              default_root_frame_push();
              rootFrames.push_back(frame);
              ++rewrittenFrameIndex;
            }
            ++frameIndex;
          }
          default_root_frame_push();

          root->children = std::move(rootFrames);
          document.frame_references_set(std::move(defaultRootSelection));
          frames_focus_sync_for(document);
        },
        SnapshotKind::TRACKS, trackReferences);
  }

  void TimelineContext::frame_split()
  {
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    if (selectedFrames.size() != 1) return;
    auto targetReference = *selectedFrames.begin();
    auto splitTime = frameSplitTimeAtCursor.value_or((int)std::floor(playback.time));
    if (targetReference.itemType == TRIGGER) return;

    edit_command_push(
        EDIT_SPLIT_FRAME, Document::FRAMES,
        [=, this](Manager&, Document& document) mutable
        {
          if (targetReference.itemType == TRIGGER) return;

          auto item = command_item_reference_get(document, targetReference);
          auto frame = command_frame_get(document, targetReference);

          if (!item || !frame) return;

          auto originalDuration = frame->duration;
          if (originalDuration <= 1) return;

          auto frameStartTime = frame_time_from_index_get(*item, targetReference.frameIndex);
          int frameStart = (int)std::round(frameStartTime);
          int firstDuration = splitTime - frameStart + 1;

          if (firstDuration <= 0 || firstDuration >= originalDuration) return;

          int secondDuration = originalDuration - firstDuration;
          auto splitFrame = *frame;
          splitFrame.duration = secondDuration;

          auto nextFrame = track_frame_get(*item, targetReference.frameIndex + 1);
          if (frame->interpolation != Interpolation::NONE && nextFrame)
            frame_mix(splitFrame, *nextFrame,
                      interpolation_factor(frame->interpolation, (float)firstDuration / (float)originalDuration));

          frame->duration = firstDuration;
          auto insertIndex = track_frame_insert_child_index_get(*item, targetReference.frameIndex + 1);
          item->children.insert(item->children.begin() + insertIndex, splitFrame);
          document.reference = targetReference;
          frames_selection_set_reference_for(document);
        },
        SnapshotKind::TRACKS, {targetReference});
  }

  void TimelineContext::item_remove()
  {
    auto targetRows = selected_row_references_get();
    std::map<int, std::set<int>> targetIds{};
    std::map<int, std::set<int>> targetGroupIds{};
    for (auto row : targetRows)
    {
      if (row.type != LAYER && row.type != NULL_) continue;
      if (row.isGroup)
        targetGroupIds[row.type].insert(row.id);
      else
        targetIds[row.type].insert(row.id);
    }
    auto animationIndex = reference.animationIndex;
    if (!animation) return;
    if (targetIds.empty() && targetGroupIds.empty()) return;
    edit_command_push(EDIT_REMOVE_ITEMS, Document::ITEMS,
                      [=, this](Manager&, Document& document) mutable
                      {
                        auto animation = document.anm2.element_get(ElementType::ANIMATION, animationIndex);
                        if (!animation) return;
                        for (auto targetType : {LAYER, NULL_})
                        {
                          auto containerType = TYPE_CONTAINERS[targetType];
                          auto targetTrackType = TYPE_TRACKS[targetType];
                          auto container = child_first_get(*animation, containerType);
                          if (!container) continue;
                          auto ids = targetIds.contains(targetType) ? targetIds.at(targetType) : std::set<int>{};
                          auto groupIds =
                              targetGroupIds.contains(targetType) ? targetGroupIds.at(targetType) : std::set<int>{};
                          for (int i = (int)container->children.size() - 1; i >= 0; --i)
                          {
                            auto& item = container->children[i];
                            if (item.type == ElementType::GROUP && groupIds.contains(item.id))
                            {
                              container->children.erase(container->children.begin() + i);
                              continue;
                            }
                            if (item.type == targetTrackType &&
                                (ids.contains(anm2ed::track_id_get(item)) || groupIds.contains(item.groupId)))
                              container->children.erase(container->children.begin() + i);
                          }
                        }
                        reference_clear_for(document);
                        group_selection_reset_for(document);
                      });
  }

  std::vector<Reference> TimelineContext::item_references_groupable_get()
  {
    if (!document.groupReferences.empty()) return std::vector<Reference>{};
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
    auto targetType = targetReferences.front().itemType;
    auto animationIndex = reference.animationIndex;
    auto groupName = std::string(localize.get(TEXT_NEW_GROUP));

    edit_command_push(EDIT_GROUP_ITEMS, Document::ITEMS,
                      [=, this](Manager&, Document& document) mutable
                      {
                        auto animation = document.anm2.element_get(ElementType::ANIMATION, animationIndex);
                        if (!animation) return;

                        auto containerType = TYPE_CONTAINERS[targetType];
                        auto targetTrackType = TYPE_TRACKS[targetType];
                        auto container = child_first_get(*animation, containerType);
                        if (!container) return;

                        std::set<int> targetIds{};
                        for (auto itemReference : targetReferences)
                          targetIds.insert(itemReference.itemID);

                        auto group = element_make(ElementType::GROUP);
                        group.id = element_child_next_id_get(*container, ElementType::GROUP);
                        group.name = groupName;
                        group.children.push_back(root_animation_make());
                        int insertIndex = (int)container->children.size();

                        for (int i = 0; i < (int)container->children.size(); ++i)
                        {
                          auto& item = container->children[i];
                          if (item.type == targetTrackType && targetIds.contains(anm2ed::track_id_get(item)))
                          {
                            insertIndex = std::min(insertIndex, i);
                            item.groupId = group.id;
                          }
                        }

                        if (insertIndex == (int)container->children.size()) return;
                        insertIndex = std::clamp(insertIndex, 0, (int)container->children.size());
                        container->children.insert(container->children.begin() + insertIndex, group);

                        document.items.references.clear();
                        for (auto itemReference : targetReferences)
                          document.items.references.insert(itemReference);
                        document.reference = targetReferences.front();
                        frames_selection_reset_for(document);
                      });
  }

  void TimelineContext::rows_move_to_row(std::vector<TimelineRowReference> draggedRows, TimelineItemRow targetRow,
                                         bool isDropAfter, bool isDropIntoGroup)
  {
    if (draggedRows.empty()) return;
    auto targetType = draggedRows.front().type;
    if (targetType != LAYER && targetType != NULL_) return;
    for (const auto& draggedRow : draggedRows)
      if (draggedRow.type != targetType) return;

    if (targetRow.isGroup && targetRow.type != targetType) return;
    if (!targetRow.isGroup && targetRow.type != targetType && targetRow.type != NONE && targetRow.type != ROOT &&
        targetRow.type != TRIGGER)
      return;

    auto animationIndex = reference.animationIndex;
    edit_command_push(
        EDIT_MOVE_ITEMS, Document::ITEMS,
        [=, this](Manager&, Document& document) mutable
        {
          auto animation = document.anm2.element_get(ElementType::ANIMATION, animationIndex);
          if (!animation) return;
          auto container = child_first_get(*animation, TYPE_CONTAINERS[targetType]);
          if (!container) return;
          auto targetTrackType = TYPE_TRACKS[targetType];

          std::set<int> draggedIds{};
          std::set<int> draggedGroupIds{};
          for (const auto& draggedRow : draggedRows)
          {
            if (draggedRow.isGroup)
              draggedGroupIds.insert(draggedRow.id);
            else
              draggedIds.insert(draggedRow.id);
          }

          if (targetRow.groupId != -1 && draggedGroupIds.contains(targetRow.groupId)) return;

          auto is_dragged = [&](const Element& item)
          {
            return (item.type == ElementType::GROUP && draggedGroupIds.contains(item.id)) ||
                   (item.type == targetTrackType &&
                    (draggedIds.contains(anm2ed::track_id_get(item)) || draggedGroupIds.contains(item.groupId)));
          };
          std::vector<Element> movedItems{};
          std::ranges::copy_if(container->children, std::back_inserter(movedItems), is_dragged);
          if (movedItems.empty()) return;

          if (targetRow.isGroup && draggedGroupIds.contains(targetRow.id)) return;

          auto row_index_get = [&](const TimelineItemRow& row)
          {
            for (int i = 0; i < (int)container->children.size(); ++i)
            {
              auto& item = container->children[i];
              if (row.isGroup && item.type == ElementType::GROUP && item.id == row.id) return i;
              if (!row.isGroup && row.type == targetType && item.type == targetTrackType &&
                  anm2ed::track_id_get(item) == row.id)
                return i;
            }
            return -1;
          };
          auto group_end_index_get = [&](int groupId)
          {
            int result = -1;
            for (int i = 0; i < (int)container->children.size(); ++i)
            {
              auto& item = container->children[i];
              if (item.type == ElementType::GROUP && item.id == groupId) result = std::max(result, i);
              if (item.type == targetTrackType && item.groupId == groupId) result = std::max(result, i);
            }
            return result;
          };

          int targetIndex = (int)container->children.size();
          if (targetRow.isGroup || targetRow.type == targetType)
          {
            auto rowIndex = row_index_get(targetRow);
            if (rowIndex == -1) return;
            if (targetRow.isGroup)
            {
              auto groupEndIndex = group_end_index_get(targetRow.id);
              if (targetType == LAYER)
                targetIndex = isDropAfter ? rowIndex : groupEndIndex + 1;
              else
                targetIndex = rowIndex + isDropAfter;
            }
            else
              targetIndex = rowIndex + (targetType == LAYER ? !isDropAfter : isDropAfter);
          }

          int removedBeforeTarget = 0;
          for (int i = (int)container->children.size() - 1; i >= 0; --i)
          {
            if (is_dragged(container->children[i]))
            {
              if (i < targetIndex) ++removedBeforeTarget;
              container->children.erase(container->children.begin() + i);
            }
          }

          auto targetGroupId = -1;
          if (targetRow.isGroup && isDropIntoGroup)
            targetGroupId = targetRow.id;
          else if (targetRow.type == targetType)
            targetGroupId = targetRow.groupId;

          for (auto& item : movedItems)
            if (item.type == targetTrackType && !draggedGroupIds.contains(item.groupId)) item.groupId = targetGroupId;

          targetIndex -= removedBeforeTarget;
          targetIndex = std::clamp(targetIndex, 0, (int)container->children.size());
          container->children.insert(container->children.begin() + targetIndex, movedItems.begin(), movedItems.end());

          document.items.references.clear();
          document.groupReferences.clear();
          for (const auto& draggedRow : draggedRows)
          {
            if (draggedRow.isGroup)
              document.groupReferences.insert({draggedRow.animationIndex, draggedRow.type, draggedRow.id});
            else
              document.items.references.insert(row_item_reference_get(draggedRow));
          }
          if (!document.items.references.empty())
            document.reference = *document.items.references.begin();
          else
            document.reference = {animationIndex};
          frames_selection_reset_for(document);
        });
  }

  void TimelineContext::fit_animation_length()
  {
    if (!animation) return;
    auto animationIndex = reference.animationIndex;
    edit_command_push(EDIT_FIT_ANIMATION_LENGTH, Document::ANIMATIONS,
                      [=, this](Manager&, Document& document)
                      {
                        auto animation = document.anm2.element_get(ElementType::ANIMATION, animationIndex);
                        if (!animation) return;
                        animation->frameNum = animation_length_get(*animation);
                      });
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
    auto trackReferences = track_references_from_frame_references_get(selectedFrames);
    edit_command_push(
        EDIT_CUT_FRAMES, Document::FRAMES, [=, this](Manager&, Document& document) mutable
        { frames_delete_for(document, selectedFrames); }, SnapshotKind::TRACKS, trackReferences);
  }

  void TimelineContext::paste()
  {
    auto targetReference = reference;
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    auto clipboardString = clipboard.get();
    if (clipboardString.empty()) return;
    auto targetHoveredTime = hoveredTime;
    auto message = std::string(localize.get(EDIT_PASTE_FRAMES));
    command_push(
        [=, this](Manager&, Document& document) mutable
        {
          auto animation = document.anm2.element_get(ElementType::ANIMATION, targetReference.animationIndex);
          if (!animation) return;
          if (auto item = command_item_reference_get(document, targetReference))
          {
            document.snapshots.tracks_push(message, {targetReference});
            std::set<int> indices{};
            std::string errorString{};
            int insertIndex = (int)item->children.size();
            std::set<int> selectedIndices{};
            for (auto frameReference : selectedFrames)
              if (is_same_item(frameReference, targetReference)) selectedIndices.insert(frameReference.frameIndex);

            if (!selectedIndices.empty())
              insertIndex = std::min((int)item->children.size(), *selectedIndices.rbegin() + 1);
            else if (targetReference.frameIndex >= 0 && targetReference.frameIndex < (int)item->children.size())
              insertIndex = targetReference.frameIndex + 1;

            auto start = targetReference.itemType == TRIGGER ? targetHoveredTime : insertIndex;
            if (frames_deserialize(*item, clipboardString, start, indices, &errorString))
            {
              if (targetReference.itemType != LAYER)
              {
                for (auto i : indices)
                  if (auto frame = track_frame_get(*item, i); frame)
                  {
                    frame->regionId = -1;
                    frame->crop = {};
                    frame->size = {};
                    frame->pivot = {};
                  }
              }
              else if (targetReference.itemID != -1)
              {
                auto layer = document.anm2.element_get(ElementType::LAYER_ELEMENT, targetReference.itemID);
                auto spritesheet =
                    layer ? document.anm2.element_get(ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;

                for (auto i : indices)
                {
                  auto frame = track_frame_get(*item, i);
                  if (!frame || frame->regionId == -1) continue;
                  auto region =
                      spritesheet ? child_id_get(*spritesheet, ElementType::REGION, frame->regionId) : nullptr;
                  if (!region) frame->regionId = -1;
                }
              }

              std::set<Reference> pastedSelection{};
              for (auto i : indices)
              {
                auto pastedReference = targetReference;
                pastedReference.frameIndex = i;
                pastedSelection.insert(pastedReference);
              }
              document.reference = targetReference;
              document.reference.frameIndex = *indices.begin();
              document.frame_references_set(std::move(pastedSelection));
              document.change(Document::FRAMES);
            }
            else
            {
              document.snapshots.pendingStep.reset();
              toasts.push(std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED), errorString));
              logger.error(
                  std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED, anm2ed::ENGLISH), errorString));
            }
          }
          else
          {
            toast_log(Level::WARNING, TOAST_DESERIALIZE_FRAMES_NO_SELECTION);
          }
        });
  }
}
