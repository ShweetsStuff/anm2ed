#include "context.hpp"

namespace anm2ed::imgui
{
  void TimelineContext::draw()
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    if (ImGui::Begin(localize.get(LABEL_TIMELINE_WINDOW), &settings.windowIsTimeline))
    {
      isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows |
                                               ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
      frames_child();
      if (frameMoveDrag.isActive && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
      {
        if (isFrameMoveDropTarget)
          frames_move_to(frameMoveDropType, frameMoveDropItemID, frameMoveDropGroupType, frameMoveDropGroupId,
                         frameMoveDropIndex);
        frame_move_drag_clear();
      }
      items_child();
    }
    ImGui::PopStyleVar();
    ImGui::End();

    if (itemProperties.update(manager, settings, document, reference)) group_selection_reset_for(document);
    group_properties_update();

    makeManyRegionsPopup.trigger();
    if (ImGui::BeginPopupModal(makeManyRegionsPopup.label(), &makeManyRegionsPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      input_text_string(localize.get(LABEL_FORMAT), &settings.generateRegionNameFormat);
      ImGui::Checkbox(localize.get(LABEL_MAP_FRAMES_TO_REGIONS), &isMakeManyRegionsMapFrames);

      auto widgetSize = widget_size_with_row_get(2);
      if (ImGui::Button(localize.get(LABEL_MAKE_MANY_REGIONS), widgetSize))
      {
        auto targetFrames = makeManyRegionReferences;
        auto format = settings.generateRegionNameFormat;
        auto mapping = isMakeManyRegionsMapFrames ? RegionFrameMapping::SET : RegionFrameMapping::PRESERVE;
        edit_command_push(EDIT_GENERATE_REGIONS_FROM_ANIMATIONS, Document::ALL,
                          [=, this](Manager&, Document& document) mutable
                          { document.anm2.regions_generate({}, targetFrames, format, mapping); });
        makeManyRegionsPopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) makeManyRegionsPopup.close();

      ImGui::EndPopup();
    }
    makeManyRegionsPopup.end();

    bakePopup.trigger();

    if (ImGui::BeginPopupModal(bakePopup.label(), &bakePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& interval = settings.bakeInterval;
      auto& isRoundRotation = settings.bakeIsRoundRotation;
      auto& isRoundScale = settings.bakeIsRoundScale;

      auto frame = frame_get();

      input_int_range(localize.get(LABEL_INTERVAL), interval, FRAME_DURATION_MIN,
                      frame ? frame->duration : FRAME_DURATION_MIN);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_INTERVAL));

      ImGui::Checkbox(localize.get(LABEL_ROUND_ROTATION), &isRoundRotation);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_ROTATION));

      ImGui::Checkbox(localize.get(LABEL_ROUND_SCALE), &isRoundScale);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_SCALE));

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_CONFIRM]);
      if (ImGui::Button(localize.get(LABEL_BAKE), widgetSize))
      {
        frames_bake();
        bakePopup.close();
      }
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_FRAMES_OPTIONS));

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) bakePopup.close();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CANCEL_BAKE_FRAMES));

      ImGui::EndPopup();
    }

    bakeIntoOtherFramesPopup.trigger();

    if (ImGui::BeginPopupModal(bakeIntoOtherFramesPopup.label(), &bakeIntoOtherFramesPopup.isOpen,
                               ImGuiWindowFlags_NoResize))
    {
      auto& isRoundRotation = settings.bakeIsRoundRotation;
      auto& isRoundScale = settings.bakeIsRoundScale;
      auto& isMatchRootInterpolation = settings.bakeIsMatchRootInterpolation;
      auto& isUseRootPivot = settings.bakeIsUseRootPivot;
      int target = (int)bakeIntoOtherFramesTarget;
      ImGui::RadioButton(localize.get(LABEL_CURRENT_SELECTION), &target,
                         (int)BakeIntoOtherFramesTarget::CURRENT_SELECTION);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_INTO_OTHER_FRAMES_CURRENT_SELECTION));
      ImGui::SameLine();
      ImGui::RadioButton(localize.get(LABEL_ALL), &target, (int)BakeIntoOtherFramesTarget::ALL);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_INTO_OTHER_FRAMES_ALL));
      bakeIntoOtherFramesTarget = (BakeIntoOtherFramesTarget)target;

      auto isCurrentSelection = bakeIntoOtherFramesTarget == BakeIntoOtherFramesTarget::CURRENT_SELECTION;
      ImGui::BeginDisabled(isCurrentSelection);
      ImGui::Checkbox(localize.get(LABEL_LAYERS), &isBakeIntoOtherFramesLayers);
      ImGui::SameLine();
      ImGui::Checkbox(localize.get(LABEL_NULLS), &isBakeIntoOtherFramesNulls);
      ImGui::EndDisabled();

      ImGui::Checkbox(localize.get(LABEL_ROUND_ROTATION), &isRoundRotation);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_ROTATION));

      ImGui::Checkbox(localize.get(LABEL_ROUND_SCALE), &isRoundScale);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_SCALE));

      ImGui::Checkbox(localize.get(LABEL_BAKE_MATCH_ROOT_INTERPOLATION), &isMatchRootInterpolation);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_MATCH_ROOT_INTERPOLATION));

      ImGui::Checkbox(localize.get(LABEL_BAKE_USE_ROOT_PIVOT), &isUseRootPivot);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_USE_ROOT_PIVOT));

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_CONFIRM]);
      ImGui::BeginDisabled(!is_bake_into_other_frames_ready());
      if (ImGui::Button(localize.get(LABEL_BAKE), widgetSize))
      {
        bake_into_other_frames();
        bakeIntoOtherFramesPopup.close();
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) bakeIntoOtherFramesPopup.close();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CANCEL_BAKE_FRAMES));

      ImGui::EndPopup();
    }

    if (animation)
    {
      if (shortcut(manager.chords[SHORTCUT_PLAY_PAUSE], shortcut::GLOBAL)) playback.toggle();

      if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_BACK], shortcut::GLOBAL))
      {
        playback_stop();
        playback.decrement(settings.playbackIsClamp ? animation->frameNum : FRAME_NUM_MAX);
        document.frameTime = playback.time;
      }

      if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_FORWARD], shortcut::GLOBAL))
      {
        playback_stop();
        playback.increment(settings.playbackIsClamp ? animation->frameNum : FRAME_NUM_MAX);
        document.frameTime = playback.time;
      }

      static bool isShortenChordHeld = false;
      auto isShortenFrame = shortcut(manager.chords[SHORTCUT_SHORTEN_FRAME], shortcut::GLOBAL);

      if (isShortenFrame)
      {

        auto selectedFrames = frame_references_for_current_get();
        std::erase_if(selectedFrames,
                      [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
        if (!selectedFrames.empty())
        {
          if (!isShortenChordHeld) frames_snapshot_command_push(EDIT_SHORTEN_FRAME, selectedFrames);
          command_push(
              [=, this](Manager&, Document& document)
              {
                for (auto frameReference : selectedFrames)
                {
                  auto frame = command_frame_get(document, frameReference);
                  if (!frame) continue;
                  frame->duration = std::max(FRAME_DURATION_MIN, frame->duration - 1);
                }
                document.change(Document::FRAMES);
              });
        }
      }
      isShortenChordHeld = isShortenFrame;

      static bool isExtendChordHeld = false;
      auto isExtendFrame = shortcut(manager.chords[SHORTCUT_EXTEND_FRAME], shortcut::GLOBAL);
      if (isExtendFrame)
      {

        auto selectedFrames = frame_references_for_current_get();
        std::erase_if(selectedFrames,
                      [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
        if (!selectedFrames.empty())
        {
          if (!isExtendChordHeld) frames_snapshot_command_push(EDIT_EXTEND_FRAME, selectedFrames);
          command_push(
              [=, this](Manager&, Document& document)
              {
                for (auto frameReference : selectedFrames)
                {
                  auto frame = command_frame_get(document, frameReference);
                  if (!frame) continue;
                  frame->duration = std::min(FRAME_DURATION_MAX, frame->duration + 1);
                }
                document.change(Document::FRAMES);
              });
        }
      }
      isExtendChordHeld = isExtendFrame;

      auto isPreviousFrame = shortcut(manager.chords[SHORTCUT_PREVIOUS_FRAME], shortcut::GLOBAL);
      auto isNextFrame = shortcut(manager.chords[SHORTCUT_NEXT_FRAME], shortcut::GLOBAL);
      auto isPreviousItem = shortcut(manager.chords[SHORTCUT_PREVIOUS_ITEM], shortcut::GLOBAL);
      auto isNextItem = shortcut(manager.chords[SHORTCUT_NEXT_ITEM], shortcut::GLOBAL);

      if (isPreviousFrame)
        if (auto item = selected_item_get(); item && !item->children.empty())
          reference.frameIndex = glm::clamp(--reference.frameIndex, 0, (int)item->children.size() - 1);

      if (isNextFrame)
        if (auto item = selected_item_get(); item && !item->children.empty())
          reference.frameIndex = glm::clamp(++reference.frameIndex, 0, (int)item->children.size() - 1);

      if (isPreviousFrame || isNextFrame)
      {
        if (auto item = selected_item_get(); item && !item->children.empty())
        {
          frames_selection_set_reference();
          document.frameTime = frame_time_from_index_get(*item, reference.frameIndex);
        }
      }

      if (isPreviousItem) reference_set_adjacent_item(-1);
      if (isNextItem) reference_set_adjacent_item(1);
    }

    if (isTextPushed) ImGui::PopStyleColor();
  }
}
