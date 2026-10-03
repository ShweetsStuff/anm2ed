#include "context.hpp"

namespace anm2ed::imgui
{
  void TimelineContext::commands_bind()
  {
    type_index = [](int type) { return std::clamp(type, 0, (int)TRIGGER); };
    item_type_get = [](int type) { return static_cast<ItemType>(type); };
    item_get = [&](int type, int id = -1, int groupType = NONE, int groupId = -1)
    { return animation ? animation_item_get(*animation, item_type_get(type), id, groupType, groupId) : nullptr; };
    frame_get = [&]()
    {
      if (reference.frameIndex < 0) return (Element*)nullptr;
      return anm2.element_get(reference);
    };
    selected_item_get = [&]()
    {
      auto itemReference = reference;
      itemReference.frameIndex = -1;
      return anm2.element_get(itemReference);
    };
    item_frame_type_get = [](const Element& item)
    { return item.type == ElementType::TRIGGERS ? ElementType::TRIGGER : ElementType::FRAME; };
    item_frames_count = [](const Element* item)
    {
      if (!item) return 0;
      auto frameType = item->type == ElementType::TRIGGERS ? ElementType::TRIGGER : ElementType::FRAME;
      int count{};
      for (const auto& child : item->children)
        if (child.type == frameType) ++count;
      return count;
    };
    item_frame_child_index_get = [](const Element& item, int frameIndex)
    {
      if (frameIndex < 0) return -1;
      auto frameType = item.type == ElementType::TRIGGERS ? ElementType::TRIGGER : ElementType::FRAME;
      int currentFrameIndex{};
      for (int i = 0; i < (int)item.children.size(); ++i)
      {
        if (item.children[i].type != frameType) continue;
        if (currentFrameIndex == frameIndex) return i;
        ++currentFrameIndex;
      }
      return -1;
    };
    item_frame_insert_index_get = [](const Element& item, int frameIndex)
    {
      auto targetFrameIndex = glm::max(0, frameIndex);
      auto frameType = item.type == ElementType::TRIGGERS ? ElementType::TRIGGER : ElementType::FRAME;
      int currentFrameIndex{};
      for (int i = 0; i < (int)item.children.size(); ++i)
      {
        if (item.children[i].type != frameType) continue;
        if (currentFrameIndex == targetFrameIndex) return i;
        ++currentFrameIndex;
      }
      return (int)item.children.size();
    };
    layer_get = [&](int id) { return anm2.element_get(ElementType::LAYER_ELEMENT, id); };
    null_get = [&](int id)
    {
      auto nulls = anm2.element_get(ElementType::NULLS);
      return nulls ? child_id_get(*nulls, ElementType::NULL_ELEMENT, id) : nullptr;
    };
    spritesheet_get = [&](int id) { return anm2.element_get(ElementType::SPRITESHEET, id); };
    info_get = [&]() { return element_first_get(anm2.root, ElementType::INFO); };
    container_type_get = [](int type)
    {
      if (type == LAYER) return ElementType::LAYER_ANIMATIONS;
      if (type == NULL_) return ElementType::NULL_ANIMATIONS;
      return ElementType::UNKNOWN;
    };
    track_type_get = [](int type)
    {
      if (type == LAYER) return ElementType::LAYER_ANIMATION;
      if (type == NULL_) return ElementType::NULL_ANIMATION;
      return ElementType::UNKNOWN;
    };
    track_id_get = [](const Element& track, int type)
    {
      if (type == LAYER) return track.layerId;
      if (type == NULL_) return track.nullId;
      return -1;
    };
    track_container_get = [&](int type)
    { return animation ? child_first_get(*animation, container_type_get(type)) : nullptr; };
    track_group_get = [&](int type, int groupId)
    {
      auto container = track_container_get(type);
      return container ? child_id_get(*container, ElementType::GROUP, groupId) : nullptr;
    };
    is_track_group_visible = [&](int type, int groupId)
    {
      if (groupId == -1) return true;
      auto group = track_group_get(type, groupId);
      return !group || group->isVisible;
    };
    row_group_get = [&](const TimelineItemRow& row) -> Element*
    { return row.isGroup ? track_group_get(row.type, row.id) : nullptr; };
    group_items_count_get = [&](int type, int groupId)
    {
      auto container = track_container_get(type);
      auto trackType = track_type_get(type);
      int count{};
      if (!container) return count;
      for (auto& item : container->children)
        if (item.type == trackType && item.groupId == groupId) ++count;
      return count;
    };

    command_animation_get = [](Document& document, int animationIndex)
    { return document.anm2.element_get(ElementType::ANIMATION, animationIndex); };
    command_item_get =
        [](Document& document, int animationIndex, int type, int id, int groupType = NONE, int groupId = -1)
    {
      auto animation = document.anm2.element_get(ElementType::ANIMATION, animationIndex);
      return animation ? animation_item_get(*animation, static_cast<ItemType>(type), id, groupType, groupId) : nullptr;
    };
    command_item_reference_get = [](Document& document, Reference itemReference)
    {
      itemReference.frameIndex = -1;
      return document.anm2.element_get(itemReference);
    };
    command_frame_get = [](Document& document, const Reference& targetReference)
    {
      if (targetReference.frameIndex < 0) return (Element*)nullptr;
      return document.anm2.element_get(targetReference);
    };
    command_layer_get = [](Document& document, int id)
    { return document.anm2.element_get(ElementType::LAYER_ELEMENT, id); };
    command_spritesheet_get = [](Document& document, int id)
    { return document.anm2.element_get(ElementType::SPRITESHEET, id); };
    command_info_get = [](Document& document) { return element_first_get(document.anm2.root, ElementType::INFO); };
    item_reference_get = [&](int type, int id, int groupType = NONE, int groupId = -1)
    { return Reference{reference.animationIndex, type, id, -1, groupType, groupId}; };
    item_reference_from_frame_get = [](Reference frameReference)
    {
      frameReference.frameIndex = -1;
      return frameReference;
    };
    is_same_item = [](const Reference& left, const Reference& right)
    {
      return left.animationIndex == right.animationIndex && left.itemType == right.itemType &&
             left.itemID == right.itemID && left.groupType == right.groupType && left.groupId == right.groupId;
    };
    is_frame_reference_valid_for = [&](Document& targetDocument, const Reference& frameReference)
    { return targetDocument.is_frame_reference_valid(frameReference); };
    group_selection_reset_for = [this](Document& targetDocument)
    {
      targetDocument.groupReferences.clear();
      isRowSelectionAnchorSet = false;
    };
    frame_references_for_current_get = [&]()
    { return document.frame_references_get(Document::FrameReferenceFallback::NONE); };
    item_references_for_current_get = [&]()
    {
      std::set<Reference> result = document.items.references;
      if (result.empty() && reference.itemType != NONE)
      {
        auto itemReference = reference;
        itemReference.frameIndex = -1;
        result.insert(itemReference);
      }
      return result;
    };
    frames_selection_sync_for = [&](Document& targetDocument)
    {
      auto selectedFrames = targetDocument.frame_references_get(Document::FrameReferenceFallback::NONE);
      targetDocument.frame_references_set(std::move(selectedFrames));
      frameSelectionSnapshot.assign(targetDocument.frames.selection.begin(), targetDocument.frames.selection.end());
      frameSelectionSnapshotReference = targetDocument.reference;
    };
    item_selection_set_for = [&](Document& targetDocument, Reference itemReference)
    {
      itemReference.frameIndex = -1;
      targetDocument.items.references = {itemReference};
    };
    frame_selection_set_for = [&](Document& targetDocument, Reference frameReference)
    {
      group_selection_reset_for(targetDocument);
      targetDocument.editTarget = Document::EditTarget::FRAME;
      targetDocument.frame_references_set({frameReference});
      frames_selection_sync_for(targetDocument);
    };
    frame_selection_toggle_for = [&](Document& targetDocument, Reference frameReference)
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
    };
    frame_selection_range_set_for = [&](Document& targetDocument, Reference firstReference, Reference lastReference,
                                        bool isAdditive) -> bool
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
    };
    all_frame_references_for_items_get = [&]() { return document.selected_item_frame_references_get(); };
    is_frame_copy_item = [](const Reference& itemReference)
    { return itemReference.itemType == ROOT || itemReference.itemType == LAYER || itemReference.itemType == NULL_; };
    copy_frame_references_get = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
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
    };
    frames_selection_reset_for = [this](Document& targetDocument)
    {
      targetDocument.frame_references_clear();
      frameSelectionSnapshot.clear();
      frameSelectionLocked.clear();
      isFrameSelectionLocked = false;
      frameFocusRequested = false;
      frameFocusIndex = -1;
      frameSelectionSnapshotReference = targetDocument.reference;
    };
    frames_selection_set_reference_for = [this](Document& targetDocument)
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
    };
    frames_reference_normalize_for = [&](Document& targetDocument)
    {
      auto selectedFrames = targetDocument.frame_references_get(Document::FrameReferenceFallback::NONE);
      if (!selectedFrames.empty())
      {
        targetDocument.frame_references_set(std::move(selectedFrames));
        frames_selection_sync_for(targetDocument);
        return;
      }

      if (targetDocument.reference.frameIndex >= 0 &&
          !is_frame_reference_valid_for(targetDocument, targetDocument.reference))
        targetDocument.reference.frameIndex = -1;
    };
    frames_reference_normalize_for(document);
    reference_clear_for = [=, this](Document& targetDocument)
    {
      targetDocument.reference = {targetDocument.reference.animationIndex};
      frames_selection_reset_for(targetDocument);
      targetDocument.items.references.clear();
    };
    reference_set_item_reference_for = [=, this](Document& targetDocument, Reference itemReference)
    {
      itemReference.frameIndex = -1;
      targetDocument.reference = itemReference;
      frames_selection_reset_for(targetDocument);
      item_selection_set_for(targetDocument, itemReference);
    };
    reference_set_timeline_item_reference_for = [=, this](Document& targetDocument, Reference itemReference)
    {
      if (itemReference.itemType == LAYER)
        if (auto layer = command_layer_get(targetDocument, itemReference.itemID))
          targetDocument.spritesheet.reference = layer->spritesheetId;
      reference_set_item_reference_for(targetDocument, itemReference);
    };
    command_push = [&](auto run)
    {
      manager.command_push(
          {manager.selected, [run](Manager& manager, Document& document) mutable { run(manager, document); }});
    };
    track_references_from_frame_references_get = [](std::set<Reference> frameReferences)
    {
      std::set<Reference> result{};
      for (auto frameReference : frameReferences)
      {
        frameReference.frameIndex = -1;
        result.insert(frameReference);
      }
      return result;
    };
    tracks_snapshot_command_push = [&](StringType messageType, const std::set<Reference>& trackReferences)
    {
      auto message = std::string(localize.get(messageType));
      auto queuedTrackReferences = trackReferences;
      manager.command_push({manager.selected, [message, queuedTrackReferences](Manager&, Document& document)
                            { document.snapshots.tracks_push(message, queuedTrackReferences); }});
    };
    frames_snapshot_command_push = [&](StringType messageType, const std::set<Reference>& frameReferences)
    {
      auto message = std::string(localize.get(messageType));
      auto queuedFrameReferences = frameReferences;
      manager.command_push({manager.selected, [message, queuedFrameReferences](Manager&, Document& document)
                            { document.snapshots.frames_push(message, queuedFrameReferences); }});
    };
    frames_edit_command_push = [&](StringType messageType, Document::ChangeType changeType,
                                   const std::set<Reference>& frameReferences, auto run)
    {
      auto message = std::string(localize.get(messageType));
      auto queuedFrameReferences = frameReferences;
      manager.command_push({manager.selected, [=, this](Manager& manager, Document& document) mutable
                            {
                              (void)changeType;
                              document.snapshots.frames_push(message, queuedFrameReferences);
                              run(manager, document);
                              document.change(changeType);
                            }});
    };
    tracks_edit_command_push = [&](StringType messageType, Document::ChangeType changeType,
                                   const std::set<Reference>& trackReferences, auto run)
    {
      auto message = std::string(localize.get(messageType));
      auto queuedTrackReferences = trackReferences;
      manager.command_push({manager.selected, [=, this](Manager& manager, Document& document) mutable
                            {
                              (void)changeType;
                              document.snapshots.tracks_push(message, queuedTrackReferences);
                              run(manager, document);
                              document.change(changeType);
                            }});
    };
    edit_command_push = [&](StringType messageType, Document::ChangeType changeType, auto run)
    {
      auto message = std::string(localize.get(messageType));
      manager.command_push({manager.selected, [=, this](Manager& manager, Document& document) mutable
                            {
                              (void)changeType;
                              document.snapshots.anm2_push(message);
                              run(manager, document);
                              document.change(changeType);
                            }});
    };
    type_color_base_vec = [&](int type)
    { return isLightTheme ? FRAME_COLOR_LIGHT_BASE[type_index(type)] : TYPE_COLOR[type]; };

    type_color_active_vec = [&](int type)
    {
      if (isLightTheme) return FRAME_COLOR_LIGHT_ACTIVE[type_index(type)];
      return TYPE_COLOR_ACTIVE[type];
    };

    type_color_hovered_vec = [&](int type)
    {
      if (isLightTheme) return FRAME_COLOR_LIGHT_HOVERED[type_index(type)];
      return TYPE_COLOR_HOVERED[type];
    };

    item_color_vec = [&](int type)
    {
      if (!isLightTheme) return TYPE_COLOR[type];
      return ITEM_COLOR_LIGHT_BASE[type_index(type)];
    };

    item_color_active_vec = [&](int type)
    {
      if (!isLightTheme) return TYPE_COLOR_ACTIVE[type];
      return ITEM_COLOR_LIGHT_ACTIVE[type_index(type)];
    };

    iconTintDefault = isLightTheme ? ICON_TINT_DEFAULT_LIGHT : ICON_TINT_DEFAULT_DARK;
    itemIconTint = isLightTheme ? ICON_TINT_DEFAULT_LIGHT : iconTintDefault;
    frameBorderColor = isLightTheme ? FRAME_BORDER_COLOR_LIGHT : FRAME_BORDER_COLOR_DARK;
    frameBorderColorReferenced =
        isLightTheme ? FRAME_BORDER_COLOR_REFERENCED_LIGHT : FRAME_BORDER_COLOR_REFERENCED_DARK;
    frameMultipleOverlayColor = isLightTheme ? FRAME_MULTIPLE_OVERLAY_COLOR_LIGHT : FRAME_MULTIPLE_OVERLAY_COLOR_DARK;
    textMultipleColor = isLightTheme ? TEXT_MULTIPLE_COLOR_LIGHT : TEXT_MULTIPLE_COLOR_DARK;
    playheadLineColor = isLightTheme ? PLAYHEAD_LINE_COLOR_LIGHT : PLAYHEAD_LINE_COLOR_DARK;
    playheadIconTint = isLightTheme ? PLAYHEAD_ICON_TINT_LIGHT : iconTintDefault;
    timelineBackgroundColor =
        isLightTheme ? TIMELINE_BACKGROUND_COLOR_LIGHT : ImGui::GetStyleColorVec4(ImGuiCol_Header);
    timelinePlayheadRectColor = isLightTheme ? TIMELINE_PLAYHEAD_RECT_COLOR_LIGHT : TIMELINE_PLAYHEAD_RECT_COLOR_DARK;
    timelineTickColor = isLightTheme ? TIMELINE_TICK_COLOR_LIGHT : frameBorderColor;
    itemTextColor = isLightTheme ? ITEM_TEXT_COLOR_LIGHT : ITEM_TEXT_COLOR_DARK;

    overlay_icon = [&](GLuint textureId, ImVec4 tint, bool isForced = false)
    {
      if (!isForced && !isLightTheme) return;
      auto min = ImGui::GetItemRectMin();
      auto max = ImGui::GetItemRectMax();
      ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)textureId, min, max, ImVec2(0, 0), ImVec2(1, 1),
                                           ImGui::GetColorU32(tint));
    };

    frames_selection_set_reference = [&]() { frames_selection_set_reference_for(document); };

    playback_stop = [&]()
    {
      playback.isPlaying = false;
      playback.isFinished = false;
      playback.timing_reset();
    };

    frame_insert = [&]()
    {
      auto targetReference = reference;
      auto targetFrameTime = document.frameTime;
      if (!animation || !command_item_reference_get(document, targetReference)) return;

      tracks_edit_command_push(EDIT_INSERT_FRAME, Document::FRAMES, {targetReference},
                               [=, this](Manager&, Document& document) mutable
                               {
                                 auto animation = command_animation_get(document, targetReference.animationIndex);
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
                                   auto framesCount = item_frames_count(item);
                                   if (frame)
                                   {
                                     auto addFrame = *frame;
                                     auto insertIndex = std::clamp(targetReference.frameIndex + 1, 0, framesCount);
                                     auto childIndex = item_frame_insert_index_get(*item, insertIndex);
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
                               });
    };

    frames_delete_for = [=, this](Document& document, std::set<Reference> selectedFrames) mutable
    {
      std::map<Reference, std::set<int>> groupedFrames{};
      for (auto frameReference : selectedFrames)
      {
        auto itemReference = item_reference_from_frame_get(frameReference);
        groupedFrames[itemReference].insert(frameReference.frameIndex);
      }

      for (auto& [itemReference, indices] : groupedFrames)
      {
        auto item = command_item_reference_get(document, itemReference);
        if (!item) continue;

        for (auto it = indices.rbegin(); it != indices.rend(); ++it)
        {
          auto childIndex = item_frame_child_index_get(*item, *it);
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
    };

    frames_delete_action = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      if (selectedFrames.empty()) return;
      auto trackReferences = track_references_from_frame_references_get(selectedFrames);
      tracks_edit_command_push(EDIT_DELETE_FRAMES, Document::FRAMES, trackReferences,
                               [=, this](Manager&, Document& document) mutable
                               { frames_delete_for(document, selectedFrames); });
    };

    frames_duplicate = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      std::erase_if(selectedFrames, [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
      if (selectedFrames.empty()) return;

      auto trackReferences = track_references_from_frame_references_get(selectedFrames);
      tracks_edit_command_push(EDIT_DUPLICATE_FRAMES, Document::FRAMES, trackReferences,
                               [=, this](Manager&, Document& document) mutable
                               {
                                 std::map<Reference, std::set<int>> groupedFrames{};
                                 for (auto frameReference : selectedFrames)
                                 {
                                   auto itemReference = item_reference_from_frame_get(frameReference);
                                   groupedFrames[itemReference].insert(frameReference.frameIndex);
                                 }

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
                                   auto childIndex = item_frame_insert_index_get(*item, insertIndex);
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
                                 frames_selection_sync_for(document);
                                 frameSelectionSnapshotReference = document.reference;
                                 frameSelectionLocked.clear();
                                 isFrameSelectionLocked = false;
                                 frameFocusIndex = document.reference.frameIndex;
                                 frameFocusRequested = true;
                               });
    };

    is_frames_reverse_available = [&](std::set<Reference> selectedFrames)
    {
      std::map<Reference, int> counts{};
      std::erase_if(selectedFrames, [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
      for (auto frameReference : selectedFrames)
      {
        auto itemReference = item_reference_from_frame_get(frameReference);
        ++counts[itemReference];
      }
      return std::ranges::any_of(counts, [](const auto& item) { return item.second >= 2; });
    };

    frames_reverse = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      std::erase_if(selectedFrames, [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
      if (!is_frames_reverse_available(selectedFrames)) return;

      auto trackReferences = track_references_from_frame_references_get(selectedFrames);
      tracks_edit_command_push(EDIT_REVERSE_FRAMES, Document::FRAMES, trackReferences,
                               [=, this](Manager&, Document& document) mutable
                               {
                                 std::map<Reference, std::set<int>> groupedFrames{};
                                 for (auto frameReference : selectedFrames)
                                 {
                                   auto itemReference = item_reference_from_frame_get(frameReference);
                                   groupedFrames[itemReference].insert(frameReference.frameIndex);
                                 }

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
                                     auto childIndex = item_frame_child_index_get(*item, *it);
                                     if (childIndex != -1) item->children.erase(item->children.begin() + childIndex);
                                   }

                                   std::ranges::reverse(reversedFrames);
                                   auto childIndex = item_frame_insert_index_get(*item, insertIndex);
                                   item->children.insert(item->children.begin() + childIndex,
                                                         std::make_move_iterator(reversedFrames.begin()),
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
                                 frames_selection_sync_for(document);
                                 frameSelectionSnapshotReference = document.reference;
                                 frameSelectionLocked.clear();
                                 isFrameSelectionLocked = false;
                                 frameFocusIndex = document.reference.frameIndex;
                                 frameFocusRequested = true;
                               });
    };

    frames_bake = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      auto bakeInterval = settings.bakeInterval;
      auto isRoundScale = settings.bakeIsRoundScale;
      auto isRoundRotation = settings.bakeIsRoundRotation;
      std::erase_if(selectedFrames, [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
      if (selectedFrames.empty()) return;

      auto trackReferences = track_references_from_frame_references_get(selectedFrames);
      tracks_edit_command_push(EDIT_BAKE_FRAMES, Document::FRAMES, trackReferences,
                               [=, this](Manager&, Document& document) mutable
                               {
                                 std::map<Reference, std::set<int>> groupedFrames{};
                                 for (auto frameReference : selectedFrames)
                                 {
                                   auto itemReference = item_reference_from_frame_get(frameReference);
                                   groupedFrames[itemReference].insert(frameReference.frameIndex);
                                 }

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

                                     auto bakedCount = originalDuration <= FRAME_DURATION_MIN
                                                           ? 1
                                                           : (int)std::ceil((float)originalDuration / bakeInterval);
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
                                   frames_selection_sync_for(document);
                                   frameSelectionSnapshotReference = document.reference;
                                   frameSelectionLocked.clear();
                                   isFrameSelectionLocked = false;
                                   frameFocusIndex = document.reference.frameIndex;
                                   frameFocusRequested = true;
                                 }
                                 else
                                   frames_selection_reset_for(document);
                               });
    };

    selected_root_frame_references_get = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      std::erase_if(selectedFrames, [](const Reference& frameReference) { return frameReference.itemType != ROOT; });
      return selectedFrames;
    };

    item_references_for_bake_into_other_frames_get = [](const Document& targetDocument, const Element& targetAnimation,
                                                        BakeIntoOtherFramesTarget target, bool isLayers, bool isNulls)
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

      auto add_items = [&](const Element& container, int itemType)
      {
        auto trackType = TYPE_TRACKS[itemType];
        auto add_item = [&](auto&& self, const Element& item) -> void
        {
          if (item.type == ElementType::GROUP)
          {
            for (const auto& child : item.children)
              self(self, child);
            return;
          }
          if (item.type != trackType) return;
          auto itemID = itemType == LAYER ? item.layerId : item.nullId;
          auto groupType = item.groupId == -1 ? NONE : itemType;
          result.insert({animationIndex, itemType, itemID, -1, groupType, item.groupId});
        };
        for (const auto& item : container.children)
          add_item(add_item, item);
      };

      if (isLayers)
        if (auto layerAnimations = child_first_get(targetAnimation, ElementType::LAYER_ANIMATIONS))
          add_items(*layerAnimations, LAYER);
      if (isNulls)
        if (auto nullAnimations = child_first_get(targetAnimation, ElementType::NULL_ANIMATIONS))
          add_items(*nullAnimations, NULL_);
      return result;
    };

    is_bake_into_other_frames_ready = [&]()
    {
      auto selectedRootFrames = selected_root_frame_references_get();
      if (selectedRootFrames.empty() || !animation) return false;
      auto targetItems = item_references_for_bake_into_other_frames_get(
          document, *animation, bakeIntoOtherFramesTarget, isBakeIntoOtherFramesLayers, isBakeIntoOtherFramesNulls);
      return !targetItems.empty();
    };

    frame_root_transform_apply =
        [](Element& frame, const Element& rootFrame, bool isRoundScale, bool isRoundRotation, bool isUseRootPivot)
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
    };

    bake_into_other_frames = [&]()
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
      tracks_edit_command_push(
          EDIT_BAKE_INTO_OTHER_FRAMES, Document::FRAMES, trackReferences,
          [=, this](Manager&, Document& document) mutable
          {
            auto animation = command_animation_get(document, document.reference.animationIndex);
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
                auto frameType = item_frame_type_get(*item);
                int frameIndex{};
                for (const auto& frame : item->children)
                {
                  if (frame.type != frameType) continue;
                  auto frameStart = (int)frame_time_from_index_get(*item, frameIndex);
                  for (auto span : spans)
                    if (frameStart >= span.start && frameStart < span.end)
                    {
                      bakeIndices.push_back(frameIndex);
                      break;
                    }
                  ++frameIndex;
                }
                for (auto it = bakeIndices.rbegin(); it != bakeIndices.rend(); ++it)
                  frame_bake(*item, *it, FRAME_DURATION_MIN, isRoundScale, isRoundRotation);
              }

              auto frameType = item_frame_type_get(*item);
              int frameIndex{};
              for (auto& frame : item->children)
              {
                if (frame.type != frameType) continue;
                auto frameStart = (int)frame_time_from_index_get(*item, frameIndex);
                bool isInsideSpan{};
                for (auto span : spans)
                  if (frameStart >= span.start && frameStart < span.end)
                  {
                    isInsideSpan = true;
                    break;
                  }
                if (!isInsideSpan)
                {
                  ++frameIndex;
                  continue;
                }
                auto rootFrame = frame_generate(*root, (float)frameStart);
                frame_root_transform_apply(frame, rootFrame, isRoundScale, isRoundRotation, isUseRootPivot);
                ++frameIndex;
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
            frames_selection_sync_for(document);
            frameSelectionSnapshotReference = document.reference;
            frameSelectionLocked.clear();
            isFrameSelectionLocked = false;
            frameFocusIndex = document.reference.frameIndex;
            frameFocusRequested = true;
          });
    };

    frame_split = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      if (selectedFrames.size() != 1) return;
      auto targetReference = *selectedFrames.begin();
      auto splitTime = frameSplitTimeAtCursor.value_or((int)std::floor(playback.time));
      if (targetReference.itemType == TRIGGER) return;

      tracks_edit_command_push(
          EDIT_SPLIT_FRAME, Document::FRAMES, {targetReference},
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
            auto insertIndex = item_frame_insert_index_get(*item, targetReference.frameIndex + 1);
            item->children.insert(item->children.begin() + insertIndex, splitFrame);
            document.reference = targetReference;
            frames_selection_set_reference_for(document);
          });
    };

    reference_clear = [&]()
    {
      group_selection_reset_for(document);
      reference_clear_for(document);
    };

    reference_set_timeline_item_reference = [&](Reference itemReference)
    {
      group_selection_reset_for(document);
      reference_set_timeline_item_reference_for(document, itemReference);
    };

    timeline_item_rows_get = [&]()
    {
      std::vector<TimelineItemRow> rows{};
      if (!animation) return rows;

      auto track_row_push = [&](const Element& item, int type, int index, int depth = 0)
      {
        auto rootGroupType = item.groupId == -1 ? NONE : type;
        rows.push_back({.type = type,
                        .id = track_id_get(item, type),
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

      if (auto layerAnimations = child_first_get(*animation, ElementType::LAYER_ANIMATIONS))
      {
        auto groupIds = group_ids_get(*layerAnimations);
        auto layer_track_push = [&](int groupId, int depth)
        {
          for (int j = (int)layerAnimations->children.size() - 1; j >= 0; --j)
          {
            auto& item = layerAnimations->children[j];
            if (item.type != ElementType::LAYER_ANIMATION || item.groupId != groupId) continue;
            if (settings.timelineIsShowUnused || !item.children.empty()) track_row_push(item, LAYER, j, depth);
          }
        };

        for (int i = (int)layerAnimations->children.size() - 1; i >= 0; --i)
        {
          auto& item = layerAnimations->children[i];
          if (item.type == ElementType::GROUP)
          {
            group_row_push(item, LAYER, i);
            if (item.isExpanded)
            {
              group_root_row_push(item, LAYER);
              layer_track_push(item.id, 1);
            }
          }
          else if (item.type == ElementType::LAYER_ANIMATION && !groupIds.contains(item.groupId) &&
                   (settings.timelineIsShowUnused || !item.children.empty()))
            track_row_push(item, LAYER, i);
        }
      }

      if (auto nullAnimations = child_first_get(*animation, ElementType::NULL_ANIMATIONS))
      {
        auto groupIds = group_ids_get(*nullAnimations);
        auto null_track_push = [&](int groupId, int depth)
        {
          for (int j = 0; j < (int)nullAnimations->children.size(); ++j)
          {
            auto& item = nullAnimations->children[j];
            if (item.type != ElementType::NULL_ANIMATION || item.groupId != groupId) continue;
            if (settings.timelineIsShowUnused || !item.children.empty()) track_row_push(item, NULL_, j, depth);
          }
        };

        for (int i = 0; i < (int)nullAnimations->children.size(); ++i)
        {
          auto& item = nullAnimations->children[i];
          if (item.type == ElementType::GROUP)
          {
            group_row_push(item, NULL_, i);
            if (item.isExpanded)
            {
              group_root_row_push(item, NULL_);
              null_track_push(item.id, 1);
            }
          }
          else if (item.type == ElementType::NULL_ANIMATION && !groupIds.contains(item.groupId) &&
                   (settings.timelineIsShowUnused || !item.children.empty()))
            track_row_push(item, NULL_, i);
        }
      }

      rows.push_back({.type = TRIGGER});
      return rows;
    };

    timeline_item_references_get = [&]()
    {
      std::vector<Reference> itemReferences;
      for (const auto& row : timeline_item_rows_get())
      {
        if (row.isGroup) continue;
        itemReferences.push_back({reference.animationIndex, row.type, row.id, -1, row.rootGroupType, row.rootGroupId});
      }
      return itemReferences;
    };

    group_reference_get = [&](const TimelineItemRow& row)
    { return Reference{reference.animationIndex, row.type, row.id}; };

    row_reference_get = [&](const TimelineItemRow& row)
    {
      return TimelineRowReference{manager.selected, reference.animationIndex, row.type,        row.id,
                                  row.index,        row.rootGroupType,        row.rootGroupId, row.isGroup};
    };

    row_item_reference_get = [](const TimelineRowReference& row)
    { return Reference{row.animationIndex, row.type, row.id, -1, row.groupType, row.groupId}; };

    timeline_row_references_get = [&]()
    {
      std::vector<TimelineRowReference> rowReferences;
      for (const auto& row : timeline_item_rows_get())
      {
        if (row.type == NONE) continue;
        rowReferences.push_back(row_reference_get(row));
      }
      return rowReferences;
    };

    is_group_selected = [&](const TimelineItemRow& row)
    { return document.groupReferences.contains(group_reference_get(row)); };

    is_row_selected = [&](const TimelineItemRow& row)
    {
      if (row.isGroup) return is_group_selected(row);
      auto itemReference = item_reference_get(row.type, row.id, row.rootGroupType, row.rootGroupId);
      auto isReferenced = is_same_item(reference, itemReference);
      return document.items.references.contains(itemReference) ||
             (document.items.references.empty() && document.groupReferences.empty() && isReferenced);
    };

    row_selection_clear = [&]()
    {
      document.items.references.clear();
      document.groupReferences.clear();
    };

    row_selection_insert = [&](const TimelineRowReference& row)
    {
      if (row.isGroup)
        document.groupReferences.insert({row.animationIndex, row.type, row.id});
      else
        document.items.references.insert(row_item_reference_get(row));
    };

    row_selection_erase = [&](const TimelineRowReference& row)
    {
      if (row.isGroup)
        document.groupReferences.erase({row.animationIndex, row.type, row.id});
      else
        document.items.references.erase(row_item_reference_get(row));
    };

    is_row_reference_selected = [&](const TimelineRowReference& row)
    {
      if (row.isGroup) return document.groupReferences.contains({row.animationIndex, row.type, row.id});
      return document.items.references.contains(row_item_reference_get(row));
    };

    row_selection_count_get = [&]() { return document.items.references.size() + document.groupReferences.size(); };

    row_selection_set = [&](const TimelineItemRow& row)
    {
      auto rowReference = row_reference_get(row);
      auto isCtrlDown = ImGui::IsKeyDown(ImGuiMod_Ctrl);
      auto isShiftDown = ImGui::IsKeyDown(ImGuiMod_Shift);

      if (row.isGroup)
        reference = {reference.animationIndex};
      else
      {
        if (row.type == LAYER)
          if (auto layer = layer_get(row.id)) document.spritesheet.reference = layer->spritesheetId;
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
    };

    reference_set_adjacent_item = [&](int direction)
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
    };

    selected_row_references_get = [&]()
    {
      std::vector<TimelineRowReference> result{};
      for (const auto& row : timeline_item_rows_get())
        if (is_row_selected(row)) result.push_back(row_reference_get(row));
      return result;
    };

    row_drag_references_get = [&](const TimelineItemRow& row)
    {
      auto clicked = row_reference_get(row);
      if (clicked.type != LAYER && clicked.type != NULL_) return std::vector<TimelineRowReference>{clicked};

      auto rows = selected_row_references_get();
      if (!is_row_reference_selected(clicked)) rows = {clicked};
      if (rows.empty()) rows = {clicked};

      std::erase_if(rows, [&](const TimelineRowReference& rowReference) { return rowReference.type != clicked.type; });
      if (rows.empty()) rows = {clicked};
      return rows;
    };

    item_remove = [&]()
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
                          auto animation = command_animation_get(document, animationIndex);
                          if (!animation) return;
                          for (auto targetType : {LAYER, NULL_})
                          {
                            auto containerType = container_type_get(targetType);
                            auto targetTrackType = track_type_get(targetType);
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
                                  (ids.contains(track_id_get(item, targetType)) || groupIds.contains(item.groupId)))
                                container->children.erase(container->children.begin() + i);
                            }
                          }
                          reference_clear_for(document);
                          group_selection_reset_for(document);
                        });
    };

    item_references_groupable_get = [&]()
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
    };

    item_group = [&]()
    {
      auto targetReferences = item_references_groupable_get();
      if (targetReferences.empty()) return;
      auto targetType = targetReferences.front().itemType;
      auto animationIndex = reference.animationIndex;
      auto groupName = std::string(localize.get(TEXT_NEW_GROUP));

      edit_command_push(EDIT_GROUP_ITEMS, Document::ITEMS,
                        [=, this](Manager&, Document& document) mutable
                        {
                          auto animation = command_animation_get(document, animationIndex);
                          if (!animation) return;

                          auto containerType = container_type_get(targetType);
                          auto targetTrackType = track_type_get(targetType);
                          auto container = child_first_get(*animation, containerType);
                          if (!container) return;

                          std::set<int> targetIds{};
                          for (auto itemReference : targetReferences)
                            targetIds.insert(itemReference.itemID);

                          auto group = element_make(ElementType::GROUP);
                          group.id = element_child_next_id_get(*container, ElementType::GROUP);
                          group.name = groupName;
                          group.isExpanded = true;
                          auto root = element_make(ElementType::ROOT_ANIMATION);
                          root.children.push_back(element_make(ElementType::FRAME));
                          group.children.push_back(root);
                          int insertIndex = (int)container->children.size();

                          for (int i = 0; i < (int)container->children.size(); ++i)
                          {
                            auto& item = container->children[i];
                            if (item.type == targetTrackType && targetIds.contains(track_id_get(item, targetType)))
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
    };

    rows_move_to_row = [&](std::vector<TimelineRowReference> draggedRows, TimelineItemRow targetRow, bool isDropAfter,
                           bool isDropIntoGroup = false)
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
            auto animation = command_animation_get(document, animationIndex);
            if (!animation) return;
            auto container = child_first_get(*animation, container_type_get(targetType));
            if (!container) return;
            auto targetTrackType = track_type_get(targetType);

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

            std::vector<Element> movedItems{};
            for (const auto& item : container->children)
            {
              auto isDraggedGroup = item.type == ElementType::GROUP && draggedGroupIds.contains(item.id);
              auto isDraggedTrack =
                  item.type == targetTrackType &&
                  (draggedIds.contains(track_id_get(item, targetType)) || draggedGroupIds.contains(item.groupId));
              if (isDraggedGroup || isDraggedTrack) movedItems.push_back(item);
            }
            if (movedItems.empty()) return;

            if (targetRow.isGroup && draggedGroupIds.contains(targetRow.id)) return;

            auto row_index_get = [&](const TimelineItemRow& row)
            {
              for (int i = 0; i < (int)container->children.size(); ++i)
              {
                auto& item = container->children[i];
                if (row.isGroup && item.type == ElementType::GROUP && item.id == row.id) return i;
                if (!row.isGroup && row.type == targetType && item.type == targetTrackType &&
                    track_id_get(item, targetType) == row.id)
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
              auto& item = container->children[i];
              auto isDraggedGroup = item.type == ElementType::GROUP && draggedGroupIds.contains(item.id);
              auto isDraggedTrack =
                  item.type == targetTrackType &&
                  (draggedIds.contains(track_id_get(item, targetType)) || draggedGroupIds.contains(item.groupId));
              if (isDraggedGroup || isDraggedTrack)
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
    };

    fit_animation_length = [&]()
    {
      if (!animation) return;
      auto animationIndex = reference.animationIndex;
      edit_command_push(EDIT_FIT_ANIMATION_LENGTH, Document::ANIMATIONS,
                        [=, this](Manager&, Document& document)
                        {
                          auto animation = command_animation_get(document, animationIndex);
                          if (!animation) return;
                          animation->frameNum = animation_length_get(*animation);
                        });
    };

    frame_references_copy = [&](const std::set<Reference>& selectedFrames)
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
    };

    copy = [&]()
    {
      auto selectedFrames = copy_frame_references_get();
      if (selectedFrames.empty()) return;

      frame_references_copy(selectedFrames);
    };

    cut = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      if (selectedFrames.empty()) return;
      frame_references_copy(selectedFrames);
      auto trackReferences = track_references_from_frame_references_get(selectedFrames);
      tracks_edit_command_push(EDIT_CUT_FRAMES, Document::FRAMES, trackReferences,
                               [=, this](Manager&, Document& document) mutable
                               { frames_delete_for(document, selectedFrames); });
    };

    paste = [&]()
    {
      auto targetReference = reference;
      auto selectedFrames = frame_references_for_current_get();
      auto clipboardString = clipboard.get();
      if (clipboardString.empty()) return;
      auto targetHoveredTime = hoveredTime;
      auto message = std::string(localize.get(EDIT_PASTE_FRAMES));
      command_push(
          [=, this](Manager&, Document& document) mutable
          {
            auto animation = command_animation_get(document, targetReference.animationIndex);
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
                  auto layer = command_layer_get(document, targetReference.itemID);
                  auto spritesheet = layer ? command_spritesheet_get(document, layer->spritesheetId) : nullptr;

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
    };

    context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      if (shortcut(manager.chords[SHORTCUT_CUT], shortcut::FOCUSED)) cut();
      if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
      if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();
      if (shortcut(manager.chords[SHORTCUT_SPLIT], shortcut::FOCUSED)) frame_split();
      if (shortcut(manager.chords[SHORTCUT_BAKE], shortcut::FOCUSED)) frames_bake();
      if (shortcut(manager.chords[SHORTCUT_FIT], shortcut::FOCUSED)) fit_animation_length();

      auto make_region = [&]()
      {
        auto targetReference = reference;
        auto frame = frame_get();
        if (!frame || targetReference.itemType != LAYER || targetReference.itemID == -1) return;
        if (frame->regionId != -1) return;
        auto layer = layer_get(targetReference.itemID);
        if (!layer) return;

        auto spritesheetID = layer->spritesheetId;
        if (!spritesheet_get(spritesheetID)) return;

        auto settingsPtr = &settings;
        command_push(
            [=, this](Manager& manager, Document& document)
            {
              auto frame = command_frame_get(document, targetReference);
              if (!frame || frame->regionId != -1) return;
              auto layer = command_layer_get(document, targetReference.itemID);
              if (!layer) return;

              auto spritesheetID = layer->spritesheetId;
              if (!command_spritesheet_get(document, spritesheetID)) return;

              auto region = element_make(ElementType::REGION);
              region.crop = frame->crop;
              region.size = frame->size;
              region.pivot = frame->pivot;
              region.origin = Origin::CUSTOM;

              document.spritesheet.reference = spritesheetID;
              settingsPtr->windowIsRegions = true;
              manager.makeRegionSpritesheetId = spritesheetID;
              manager.makeRegion = region;
              manager.isMakeRegionRequested = true;
            });
      };

      auto make_many_regions = [&]()
      {
        auto targetFrames = frame_references_for_current_get();
        std::erase_if(targetFrames,
                      [&](const Reference& frameReference)
                      {
                        if (frameReference.itemType != LAYER || frameReference.frameIndex < 0) return true;
                        auto item = item_get(frameReference.itemType, frameReference.itemID, frameReference.groupType,
                                             frameReference.groupId);
                        auto frame = item ? track_frame_get(*item, frameReference.frameIndex) : nullptr;
                        auto layer = layer_get(frameReference.itemID);
                        return !frame || frame->regionId != -1 || !layer || !spritesheet_get(layer->spritesheetId);
                      });
        if (targetFrames.size() < 2) return;

        makeManyRegionReferences = targetFrames;
        makeManyRegionsPopup.open();
      };

      auto item = selected_item_get();
      auto frame = frame_get();
      auto selectedFrames = frame_references_for_current_get();
      auto copyFrames = copy_frame_references_get();
      auto selectedBakeFrames = selectedFrames;
      std::erase_if(selectedBakeFrames,
                    [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
      auto isReverseFrames = is_frames_reverse_available(selectedFrames);
      auto selectedRegionFrames = selectedFrames;
      std::erase_if(selectedRegionFrames,
                    [&](const Reference& frameReference)
                    {
                      if (frameReference.itemType != LAYER || frameReference.frameIndex < 0) return true;
                      auto item = item_get(frameReference.itemType, frameReference.itemID, frameReference.groupType,
                                           frameReference.groupId);
                      auto frame = item ? track_frame_get(*item, frameReference.frameIndex) : nullptr;
                      auto layer = layer_get(frameReference.itemID);
                      return !frame || frame->regionId != -1 || !layer || !spritesheet_get(layer->spritesheetId);
                    });
      bool isMakeManyRegions = selectedRegionFrames.size() > 1;
      auto selectedRootFrames = selected_root_frame_references_get();
      bool isMakeRegion = frame && reference.itemType == LAYER && reference.itemID != -1 && frame->regionId == -1 &&
                          layer_get(reference.itemID) && spritesheet_get(layer_get(reference.itemID)->spritesheetId);
      Actions actions{};
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
      actions.add({.label = playback.isPlaying ? LABEL_PAUSE : LABEL_PLAY,
                   .shortcut = SHORTCUT_PLAY_PAUSE,
                   .isEnabled = []() { return true; },
                   .run = [&]() { playback.toggle(); }});
      actions.add({.label = LABEL_INSERT,
                   .shortcut = SHORTCUT_INSERT_FRAME,
                   .isEnabled = [&]() { return item; },
                   .run = [&]() { frame_insert(); }});
      actions.add(ACTION_DUPLICATE, [=, this]() { return !selectedBakeFrames.empty(); }, [&]() { frames_duplicate(); });
      actions.add({.label = LABEL_SPLIT,
                   .shortcut = SHORTCUT_SPLIT,
                   .isEnabled = [=, this]() { return selectedBakeFrames.size() == 1; },
                   .run = [&]() { frame_split(); }});
      actions.add(ACTION_REVERSE, [=, this]() { return isReverseFrames; }, [&]() { frames_reverse(); });
      actions.separator();
      actions.add({.label = LABEL_BAKE,
                   .shortcut = SHORTCUT_BAKE,
                   .isEnabled = [=, this]() { return !selectedBakeFrames.empty(); },
                   .run = [&]() { frames_bake(); }});
      actions.add({.label = LABEL_BAKE_INTO_OTHER_FRAMES,
                   .shortcut = -1,
                   .isEnabled = [=, this]() { return !selectedRootFrames.empty(); },
                   .run = [&]() { bakeIntoOtherFramesPopup.open(); }});
      actions.add({.label = LABEL_FIT_ANIMATION_LENGTH,
                   .shortcut = SHORTCUT_FIT,
                   .isEnabled = [&]() { return animation && animation->frameNum != animation_length_get(*animation); },
                   .run = [&]() { fit_animation_length(); }});
      actions.separator();
      actions.add({.label = isMakeManyRegions ? LABEL_MAKE_MANY_REGIONS : LABEL_MAKE_REGION,
                   .shortcut = -1,
                   .isEnabled = [=, this]() { return isMakeManyRegions || isMakeRegion; },
                   .run =
                       [&]()
                   {
                     if (isMakeManyRegions)
                       make_many_regions();
                     else
                       make_region();
                   }});
      actions.separator();
      actions.add({.label = LABEL_DELETE,
                   .shortcut = SHORTCUT_REMOVE,
                   .isEnabled = [=, this]() { return !selectedFrames.empty(); },
                   .run = [&]() { frames_delete_action(); }});
      actions.separator();
      actions.add(ACTION_CUT, [=, this]() { return !selectedFrames.empty(); }, cut);
      actions.add(ACTION_COPY, [=, this]() { return !copyFrames.empty(); }, copy);
      actions.add(ACTION_PASTE, [&]() { return !clipboard.is_empty(); }, paste);
      actions_context_window_draw("##Context Menu", actions, settings);

      ImGui::PopStyleVar(2);
    };

    item_base_properties_open = [&](int type, int id)
    {
      if (auto row = anm2ed::track_container_get((ItemType)type)) manager.item_properties_open(row->element, id);
    };

    group_properties_close = [&]()
    {
      groupName.clear();
      groupAnimationIndex = -1;
      groupType = NONE;
      groupId = -1;
      groupPropertiesPopup.close();
    };

    group_properties_open = [&](const TimelineItemRow& row, const Element& group)
    {
      groupName = group.name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : group.name;
      groupAnimationIndex = reference.animationIndex;
      groupType = row.type;
      groupId = row.id;
      groupPropertiesPopup.open();
    };

    group_properties_update = [&]()
    {
      groupPropertiesPopup.trigger();

      if (ImGui::BeginPopupModal(groupPropertiesPopup.label(), &groupPropertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto childSize = child_size_get(1);
        if (ImGui::BeginChild("##Group Properties Child", childSize, ImGuiChildFlags_Borders))
        {
          if (groupPropertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
          input_text_string(localize.get(BASIC_NAME), &groupName);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
        }
        ImGui::EndChild();

        auto widgetSize = widget_size_with_row_get(2);

        shortcut(manager.chords[SHORTCUT_CONFIRM]);
        if (ImGui::Button(localize.get(BASIC_CONFIRM), widgetSize))
        {
          auto targetName = groupName;
          auto targetAnimationIndex = groupAnimationIndex;
          auto targetType = groupType;
          auto targetId = groupId;
          edit_command_push(EDIT_RENAME_GROUP, Document::ITEMS,
                            [=, this](Manager&, Document& document)
                            {
                              auto animation = command_animation_get(document, targetAnimationIndex);
                              auto container =
                                  animation ? child_first_get(*animation, container_type_get(targetType)) : nullptr;
                              auto group = container ? child_id_get(*container, ElementType::GROUP, targetId) : nullptr;
                              if (!group) return;
                              group->name = targetName;
                            });
          group_properties_close();
        }

        ImGui::SameLine();

        shortcut(manager.chords[SHORTCUT_CANCEL]);
        if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) group_properties_close();

        ImGui::EndPopup();
      }

      groupPropertiesPopup.end();
    };

    item_context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      auto& type = reference.itemType;
      auto& id = reference.itemID;
      auto item = selected_item_get();
      auto selectedRows = selected_row_references_get();
      auto selectedGroupableItems = item_references_groupable_get();
      auto copyFrames = copy_frame_references_get();
      TimelineItemRow selectedGroupRow{};
      Element* selectedGroup{};
      if (selectedRows.size() == 1 && selectedRows.front().isGroup)
      {
        auto row = selectedRows.front();
        selectedGroupRow = {.type = row.type, .id = row.id, .index = row.index, .isGroup = true};
        selectedGroup = row_group_get(selectedGroupRow);
      }
      auto isRemoveAvailable = std::ranges::any_of(selectedRows, [](const TimelineRowReference& row)
                                                   { return row.type == LAYER || row.type == NULL_; });
      auto item_cut = [&]()
      {
        if (copyFrames.empty() || !isRemoveAvailable) return;
        frame_references_copy(copyFrames);
        item_remove();
      };

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##Items Context Menu");

      Actions actions{};
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
      actions.add(
          ACTION_PROPERTIES, [&]() { return selectedGroup || (item && (type == LAYER || type == NULL_)); },
          [&]()
          {
            if (selectedGroup)
              group_properties_open(selectedGroupRow, *selectedGroup);
            else
              item_base_properties_open(type, id);
          });
      actions.add(ACTION_ADD, [&]() { return animation; }, [&]() { itemProperties.open(); });
      actions.add(ACTION_REMOVE, [=, this]() { return isRemoveAvailable; }, [&]() { item_remove(); });
      actions.add(ACTION_GROUP, [=, this]() { return !selectedGroupableItems.empty(); }, [&]() { item_group(); });
      actions.separator();
      actions.add(ACTION_CUT, [=, this]() { return !copyFrames.empty() && isRemoveAvailable; }, item_cut);
      actions.add(ACTION_COPY, [=, this]() { return !copyFrames.empty(); }, copy);
      actions.add(ACTION_PASTE, [&]() { return item && !clipboard.is_empty(); }, paste);
      if (shortcut(manager.chords[SHORTCUT_CUT], shortcut::FOCUSED) && !copyFrames.empty() && isRemoveAvailable)
        item_cut();
      if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED) && !copyFrames.empty()) copy();
      if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED) && item && !clipboard.is_empty()) paste();
      actions_popup_draw("##Items Context Menu", actions, settings);

      ImGui::PopStyleVar(2);
    };
  }
}
