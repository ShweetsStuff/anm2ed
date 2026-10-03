#include "context.hpp"

namespace anm2ed::imgui
{
  void TimelineContext::frame_move_drag_clear()
  {
    frameMoveDrag = {};
    frameMoveDropType = NONE;
    frameMoveDropItemID = -1;
    frameMoveDropGroupType = NONE;
    frameMoveDropGroupId = -1;
    frameMoveDropIndex = -1;
    isFrameMoveDropTarget = false;
  }

  void TimelineContext::frames_move_to(int targetType, int targetID, int targetGroupType, int targetGroupId,
                                       int insertIndex)
  {
    if (!frameMoveDrag.isActive || !animation || frameMoveDrag.animationIndex != reference.animationIndex) return;
    if (targetType == TRIGGER) return;

    auto drag = frameMoveDrag;
    std::erase_if(drag.references, [](const Reference& frameReference)
                  { return frameReference.itemType == TRIGGER || frameReference.frameIndex < 0; });
    if (drag.references.empty() && drag.frameIndex >= 0)
      drag.references.push_back(
          {drag.animationIndex, drag.type, drag.itemID, drag.frameIndex, drag.groupType, drag.groupId});
    std::erase_if(drag.references, [](const Reference& frameReference)
                  { return frameReference.itemType == TRIGGER || frameReference.frameIndex < 0; });
    if (drag.references.empty()) return;

    Reference target{drag.animationIndex, targetType, targetID, -1, targetGroupType, targetGroupId};
    edit_push(
        EDIT_MOVE_FRAMES,
        [frames = std::set<Reference>(drag.references.begin(), drag.references.end()), target,
         insertIndex](model::Model& model) { return edit::frames_move(model, frames, target, insertIndex); },
        [=, this](Document& document, const edit::Uids& uids)
        {
          frames_select_for(document, uids);
          if (targetType == LAYER)
            if (auto layer = model::item_get(document.model.content.layers, targetID))
              document.focused_id_set(SelectionKind::SPRITESHEETS, layer->spritesheetId);
        });
  }

  ImVec2 TimelineContext::frame_box_content_point_get()
  {
    auto mousePos = ImGui::GetIO().MousePos;
    return ImVec2(mousePos.x + scroll.x - frameBoxClipMin.x, mousePos.y + scroll.y - frameBoxClipMin.y);
  }

  ImVec2 TimelineContext::frame_box_screen_point_get(ImVec2 point)
  {
    return ImVec2(point.x - scroll.x + frameBoxClipMin.x, point.y - scroll.y + frameBoxClipMin.y);
  }

  bool TimelineContext::is_frame_box_overlapping(ImVec2 leftMin, ImVec2 leftMax, ImVec2 rightMin, ImVec2 rightMax)
  {
    return leftMin.x <= rightMax.x && leftMax.x >= rightMin.x && leftMin.y <= rightMax.y && leftMax.y >= rightMin.y;
  }

  void TimelineContext::frame_overlay_draw(ImDrawList* drawList, ImVec2 clipMin, ImVec2 clipMax)
  {
    if (isFrameBoxSelecting && isFrameBoxClipSet)
    {
      auto boxContentMin = ImVec2(std::min(frameBoxStart.x, frameBoxEnd.x), std::min(frameBoxStart.y, frameBoxEnd.y));
      auto boxContentMax = ImVec2(std::max(frameBoxStart.x, frameBoxEnd.x), std::max(frameBoxStart.y, frameBoxEnd.y));
      auto boxMin = frame_box_screen_point_get(boxContentMin);
      auto boxMax = frame_box_screen_point_get(boxContentMax);
      auto boxClipMin = clipMin;
      if (isPlayheadLineSet) boxClipMin.y = std::max(boxClipMin.y, playheadLineTopY);
      drawList->PushClipRect(boxClipMin, clipMax, true);
      drawList->AddRectFilled(boxMin, boxMax, ImGui::GetColorU32(ImGuiCol_DragDropTargetBg));
      drawList->AddRect(boxMin, boxMax, ImGui::GetColorU32(ImGuiCol_DragDropTarget));
      drawList->PopClipRect();
    }

    if (isPlayheadLineSet)
    {
      auto lineTopY = std::max(clipMin.y, playheadLineTopY);
      if (clipMax.y > lineTopY)
      {
        auto linePos = ImVec2(playheadLineCenterX - (PLAYHEAD_LINE_THICKNESS * 0.5f), lineTopY);
        auto lineSize = ImVec2(PLAYHEAD_LINE_THICKNESS * 0.5f, clipMax.y - lineTopY);
        drawList->PushClipRect(clipMin, clipMax, true);
        drawList->AddRectFilled(linePos, ImVec2(linePos.x + lineSize.x, linePos.y + lineSize.y),
                                ImGui::GetColorU32(playheadLineColor));
        drawList->PopClipRect();
      }
    }
  }

  void TimelineContext::frame_child(const TimelineItemRow& row, int& index, float width)
  {
    auto type = row.type;
    auto id = row.id;
    auto row_reference_make = [&](int frameIndex = -1)
    { return Reference{reference.animationIndex, type, id, frameIndex, row.rootGroupType, row.rootGroupId}; };
    auto childSize = ImVec2(width, rowFrameChildHeight);
    if (row.isGroup)
    {
      auto group = row_group_get(row);
      if (!group) return;

      ImGui::PushID(index);
      ImGui::BeginChild("##Frames Group Child", childSize, ImGuiChildFlags_None,
                        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
      ImGui::EndChild();
      index++;
      ImGui::PopID();
      return;
    }

    auto item = item_get(type, id, row.rootGroupType, row.rootGroupId);
    if (type != NONE && !item) return;

    auto isVisible = item ? item->isVisible && is_track_group_visible(type, row.groupId) : false;
    if (item && type == ROOT && row.rootGroupId != -1)
      isVisible = item->isVisible && is_track_group_visible(row.rootGroupType, row.rootGroupId);
    auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
    if (isOnlyShowLayers && type != LAYER) isVisible = false;

    auto colorVec = color_get(COLOR_FRAME_BASE, type);
    auto colorActiveVec = color_get(COLOR_FRAME_ACTIVE, type);
    auto colorHoveredVec = color_get(COLOR_FRAME_HOVERED, type);
    auto color = to_imvec4(colorVec);
    auto colorActive = to_imvec4(colorActiveVec);
    auto colorHovered = to_imvec4(colorHoveredVec);
    auto colorHidden = to_imvec4(colorVec * COLOR_HIDDEN_MULTIPLIER);
    auto colorActiveHidden = to_imvec4(colorActiveVec * COLOR_HIDDEN_MULTIPLIER);
    auto colorHoveredHidden = to_imvec4(colorHoveredVec * COLOR_HIDDEN_MULTIPLIER);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

    ImGui::PopStyleVar(2);

    ImGui::PushID(index);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
    bool isDefaultChild = type == NONE;
    if (isLightTheme && isDefaultChild) ImGui::PushStyleColor(ImGuiCol_ChildBg, TIMELINE_CHILD_BG_COLOR_LIGHT);

    bool isFramesChildVisible = ImGui::BeginChild("##Frames Child", childSize, ImGuiChildFlags_Borders);
    auto drawList = ImGui::GetWindowDrawList();
    auto clipMax = drawList->GetClipRectMax();
    auto length = animation ? animation->frameNum : 0;
    auto frameSize = ImVec2(ImGui::GetTextLineHeight(), ImGui::GetContentRegionAvail().y);
    auto framesSize = ImVec2(frameSize.x * length, frameSize.y);
    auto cursorPos = ImGui::GetCursorPos();
    auto cursorScreenPos = ImGui::GetCursorScreenPos();
    if (frameSize.x > 0.0f && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
      auto mouseX = ImGui::GetIO().MousePos.x - cursorScreenPos.x;
      frameSplitTimeAtCursor = glm::max(0, (int)std::floor(mouseX / frameSize.x));
    }
    auto border = glm::max(0.5f, ImGui::GetStyle().FrameBorderSize * 0.5f);
    auto borderLineLength = frameSize.y / 5;
    auto frameMin = std::max(0, (int)std::floor(scroll.x / frameSize.x) - 1);
    auto frameMax = std::min(FRAME_NUM_MAX, (int)std::ceil((scroll.x + clipMax.x) / frameSize.x) + 1);

    if (isFrameBoxSelecting && isFrameBoxClipSet && type != NONE && animation && item)
    {
      auto boxMin = ImVec2(std::min(frameBoxStart.x, frameBoxEnd.x), std::min(frameBoxStart.y, frameBoxEnd.y));
      auto boxMax = ImVec2(std::max(frameBoxStart.x, frameBoxEnd.x), std::max(frameBoxStart.y, frameBoxEnd.y));
      auto rowMinY = cursorScreenPos.y + scroll.y - frameBoxClipMin.y;
      auto rowMaxY = rowMinY + childSize.y;
      float selectionFrameTime{};
      int frameIndex{};
      for (const auto& frame : item->frames)
      {
        auto frameReference = row_reference_make(frameIndex);
        auto frameStart = type == TRIGGER ? frame.atFrame : selectionFrameTime;
        auto frameEnd = type == TRIGGER ? frameStart + 1.0f : frameStart + frame.duration;
        auto frameContentMin = ImVec2(frameStart * frameSize.x, rowMinY);
        auto frameContentMax = ImVec2(frameEnd * frameSize.x, rowMaxY);
        if (is_frame_box_overlapping(frameContentMin, frameContentMax, boxMin, boxMax))
          frameBoxSelection.insert(frameReference);
        if (type != TRIGGER) selectionFrameTime += frame.duration;
        ++frameIndex;
      }
    }

    if (isFramesChildVisible)
    {
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
          ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused))
      {
        if (!document.selection.uids[SelectionKind::FRAMES].empty())
        {
          reference_set(item_reference_from_frame_get(reference));
          frames_selection_reset_for(document);
        }
        else if (reference.itemType != NONE || reference.itemID != -1)
          reference_clear();
      }

      if (type == NONE)
      {
        if (length > 0)
        {
          if (isLightTheme)
          {
            auto totalMax = ImVec2(cursorScreenPos.x + framesSize.x, cursorScreenPos.y + framesSize.y);
            drawList->AddRectFilled(cursorScreenPos, totalMax, ImGui::GetColorU32(timelineBackgroundColor));
            float animationWidth = std::min(framesSize.x, frameSize.x * (float)length);
            drawList->AddRectFilled(cursorScreenPos,
                                    ImVec2(cursorScreenPos.x + animationWidth, cursorScreenPos.y + framesSize.y),
                                    ImGui::GetColorU32(timelinePlayheadRectColor));
          }
          else
          {
            drawList->AddRectFilled(cursorScreenPos,
                                    ImVec2(cursorScreenPos.x + framesSize.x, cursorScreenPos.y + framesSize.y),
                                    ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Header)));
          }
        }

        for (int i = frameMin; i < frameMax; i++)
        {
          auto frameScreenPos = ImVec2(cursorScreenPos.x + frameSize.x * (float)i, cursorScreenPos.y);

          drawList->AddRect(frameScreenPos, ImVec2(frameScreenPos.x + border, frameScreenPos.y + borderLineLength),
                            ImGui::GetColorU32(timelineTickColor), 0, 0, 0.5f);

          drawList->AddRect(ImVec2(frameScreenPos.x, frameScreenPos.y + frameSize.y - borderLineLength),
                            ImVec2(frameScreenPos.x + border, frameScreenPos.y + frameSize.y),
                            ImGui::GetColorU32(timelineTickColor), 0, 0, 0.5);

          if (i % FRAME_MULTIPLE == 0)
          {
            auto string = std::to_string(i);
            auto textSize = ImGui::CalcTextSize(string.c_str());
            auto textPos = ImVec2(frameScreenPos.x + (frameSize.x - textSize.x) / 2,
                                  frameScreenPos.y + (frameSize.y - textSize.y) / 2);

            drawList->AddRectFilled(frameScreenPos,
                                    ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y),
                                    ImGui::GetColorU32(frameMultipleOverlayColor));

            drawList->AddText(textPos, ImGui::GetColorU32(textMultipleColor), string.c_str());
          }
        }

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
          playback_stop();
          isDragging = true;
        }

        auto childPos = ImGui::GetWindowPos();
        auto mousePos = ImGui::GetIO().MousePos;
        auto localMousePos = ImVec2(mousePos.x - childPos.x, mousePos.y - childPos.y);
        hoveredTime = floorf(localMousePos.x / frameSize.x);

        if (isDragging)
        {
          playback.time = hoveredTime;
          playback.clamp(settings.playbackIsClamp ? length : FRAME_NUM_MAX);
          document.frameTime = playback.time;
        }

        if (!playback.isPlaying) playback.clamp(settings.playbackIsClamp ? length : FRAME_NUM_MAX);

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) isDragging = false;

        if (length > 0)
        {
          ImGui::SetCursorPos(ImVec2(cursorPos.x + frameSize.x * floorf(playback.time), cursorPos.y));
          ImGui::Image(resources.icon_id_get(icon::PLAYHEAD), frameSize);
          auto playheadMin = ImGui::GetItemRectMin();
          auto playheadMax = ImGui::GetItemRectMax();
          playheadLineCenterX = (playheadMin.x + playheadMax.x) * 0.5f;
          playheadLineTopY = playheadMax.y;
          isPlayheadLineSet = true;
          overlay_icon(resources.icon_id_get(icon::PLAYHEAD), playheadIconTint, true);
        }
      }
      else if (animation)
      {
        float frameTime{};

        for (int i = frameMin; i < frameMax; i++)
        {
          auto frameScreenPos = ImVec2(cursorScreenPos.x + frameSize.x * (float)i, cursorScreenPos.y);
          auto frameRectMax = ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y);

          drawList->AddRect(frameScreenPos, frameRectMax, ImGui::GetColorU32(frameBorderColor));

          if (i % FRAME_MULTIPLE == 0)
            drawList->AddRectFilled(frameScreenPos, frameRectMax, ImGui::GetColorU32(frameMultipleOverlayColor));
        }

        if (!frameMoveDrag.isActive && !ImGui::IsKeyDown(ImGuiMod_Shift) &&
            ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
        {
          isFrameBoxPending = true;
          isFrameBoxAdditive = ImGui::IsKeyDown(ImGuiMod_Ctrl);
          frameBoxStart = frame_box_content_point_get();
          frameBoxEnd = frameBoxStart;
          frameBoxSelection.clear();
        }

        bool isFrameMovePreview = false;
        ImVec2 frameMovePreviewMin{};
        ImVec2 frameMovePreviewMax{};
        bool isFrameMoveHoveredFrame = false;
        ImVec2 frameMoveHoveredFrameMin{};
        ImVec2 frameMoveHoveredFrameMax{};

        if (frameMoveDrag.isActive && type != TRIGGER)
        {
          auto mousePos = ImGui::GetIO().MousePos;
          auto rowMin = cursorScreenPos;
          auto rowMax = ImVec2(cursorScreenPos.x + width, cursorScreenPos.y + frameSize.y);
          if (mousePos.x >= rowMin.x && mousePos.x < rowMax.x && mousePos.y >= rowMin.y && mousePos.y < rowMax.y)
          {
            auto mouseX = mousePos.x - cursorScreenPos.x;
            auto targetTime = glm::max(0.0f, mouseX / frameSize.x);
            int dropIndex = (int)item->frames.size();
            float dropFrameTime{};
            float frameTime{};
            int frameIndex{};

            for (const auto& frame : item->frames)
            {
              auto frameStart = frameTime;
              auto frameEnd = frameStart + frame.duration;
              if (!isFrameMoveHoveredFrame && targetTime >= frameStart && targetTime < frameEnd)
              {
                isFrameMoveHoveredFrame = true;
                frameMoveHoveredFrameMin = ImVec2(cursorScreenPos.x + frameStart * frameSize.x, cursorScreenPos.y);
                frameMoveHoveredFrameMax =
                    ImVec2(cursorScreenPos.x + frameEnd * frameSize.x, cursorScreenPos.y + frameSize.y);
              }

              auto midpoint = frameStart + ((float)frame.duration * 0.5f);
              if (targetTime < midpoint)
              {
                dropIndex = frameIndex;
                dropFrameTime = frameStart;
                break;
              }

              frameTime = frameEnd;
              dropFrameTime = frameTime;
              ++frameIndex;
            }

            frameMoveDropType = type;
            frameMoveDropItemID = id;
            frameMoveDropGroupType = row.rootGroupType;
            frameMoveDropGroupId = row.rootGroupId;
            frameMoveDropIndex = dropIndex;
            isFrameMoveDropTarget = true;

            auto dropX = cursorScreenPos.x + dropFrameTime * frameSize.x;
            auto previewWidth = glm::max(frameSize.x, (float)frameMoveDrag.duration * frameSize.x);
            frameMovePreviewMin = ImVec2(dropX, cursorScreenPos.y);
            frameMovePreviewMax = ImVec2(dropX + previewWidth, cursorScreenPos.y + frameSize.y);
            isFrameMovePreview = true;
          }
        }

        auto& selectedFrames = document.selection.uids[SelectionKind::FRAMES];
        for (int frameIndex = 0; frameIndex < (int)item->frames.size(); ++frameIndex)
        {
          auto& frame = item->frames[frameIndex];
          ImGui::PushID(frameIndex);

          auto frameReference = row_reference_make(frameIndex);
          auto isFrameVisible = isVisible && frame.isVisible;
          auto isReferenced = reference == frameReference;
          auto isSelected = selectedFrames.contains(frame.uid);

          if (type == TRIGGER) frameTime = frame.atFrame;

          auto buttonSize = type == TRIGGER ? frameSize : to_imvec2(vec2(frameSize.x * frame.duration, frameSize.y));
          auto frameStart = type == TRIGGER ? frame.atFrame : frameTime;
          auto frameEnd = type == TRIGGER ? frameStart + 1.0f : frameStart + frame.duration;
          if (frameEnd <= (float)frameMin || frameStart >= (float)frameMax)
          {
            if (type != TRIGGER) frameTime += frame.duration;
            ImGui::PopID();
            continue;
          }
          auto buttonPos = ImVec2(cursorPos.x + (frameTime * frameSize.x), cursorPos.y);

          ImGui::SetCursorPos(buttonPos);

          auto buttonScreenPos = ImGui::GetCursorScreenPos();
          auto fillColor =
              isSelected ? (isFrameVisible ? colorActive : colorActiveHidden) : (isFrameVisible ? color : colorHidden);
          drawList->AddRectFilled(buttonScreenPos,
                                  ImVec2(buttonScreenPos.x + buttonSize.x, buttonScreenPos.y + buttonSize.y),
                                  ImGui::GetColorU32(fillColor), FRAME_ROUNDING);

          ImGui::PushStyleColor(ImGuiCol_Header, isFrameVisible ? colorActive : colorActiveHidden);
          ImGui::PushStyleColor(ImGuiCol_HeaderActive, isFrameVisible ? colorActive : colorActiveHidden);
          ImGui::PushStyleColor(ImGuiCol_HeaderHovered, isFrameVisible ? colorHovered : colorHoveredHidden);
          ImGui::PushStyleColor(ImGuiCol_NavCursor, isFrameVisible ? colorHovered : colorHoveredHidden);

          ImGui::SetNextItemAllowOverlap();
          ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING);
          ImGui::SetNextItemSelectionUserData(frameIndex);
          if (ImGui::Selectable("##Frame Button", isSelected, ImGuiSelectableFlags_None, buttonSize))
          {
            if (type == LAYER)
              if (auto layer = model::item_get(model.content.layers, id))
                document.focused_id_set(SelectionKind::SPRITESHEETS, layer->spritesheetId);

            if (type != TRIGGER)
            {
              if (ImGui::IsKeyDown(ImGuiMod_Alt))
              {
                auto targetReference = frameReference;
                edit_push(EDIT_FRAME_INTERPOLATION,
                          [=](model::Model& model)
                          {
                            if (auto frame = model.frame_edit(targetReference))
                              frame->interpolation = frame->interpolation == Interpolation::NONE ? Interpolation::LINEAR
                                                                                                 : Interpolation::NONE;
                          });
              }

              document.frameTime = frameTime;
            }

            auto isCtrlDown = ImGui::IsKeyDown(ImGuiMod_Ctrl);
            auto isShiftDown = ImGui::IsKeyDown(ImGuiMod_Shift);
            if (isShiftDown)
            {
              auto isHadAnchor = isFrameSelectionAnchorSet;
              auto anchorReference = isHadAnchor ? frameSelectionAnchor : frameReference;
              auto isRangeSelected =
                  frame_selection_range_set_for(document, anchorReference, frameReference, isCtrlDown);
              if (!isRangeSelected) frame_selection_set_for(document, frameReference);
              if (!isHadAnchor || !isRangeSelected) frameSelectionAnchor = frameReference;
              isFrameSelectionAnchorSet = true;
            }
            else if (isCtrlDown)
            {
              frame_selection_toggle_for(document, frameReference);
              frameSelectionAnchor = frameReference;
              isFrameSelectionAnchorSet = true;
            }
            else
            {
              frame_selection_set_for(document, frameReference);
              frameSelectionAnchor = frameReference;
              isFrameSelectionAnchorSet = true;
            }
            reference_set(frameReference);
            isReferenced = true;
            document.selected_clear(SelectionKind::REGIONS);
          }
          ImGui::PopStyleVar();

          ImGui::PopStyleColor(4);

          if (!isDraggedFrameActive && ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
          {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
              if (type == TRIGGER || ImGui::IsKeyDown(ImGuiMod_Ctrl))
              {
                isDraggedFrameActive = true;
                draggedFrameReference = frameReference;
                draggedFrameType = type;
                draggedFrameIndex = frameIndex;
                draggedFrameStart = hoveredTime;
                if (type != TRIGGER) draggedFrameStartDuration = frame.duration;
                draggedFrameStartDurations.clear();
                if (type != TRIGGER)
                  for (auto selectedReference : drag_frame_references_get(frameReference))
                    if (auto selectedFrame = command_frame_get(document, selectedReference))
                      draggedFrameStartDurations.push_back({selectedReference, selectedFrame->duration});
                draggedFrameStartMouseX = ImGui::GetIO().MousePos.x;
                draggedFrameWidth = frameSize.x;
              }
            }
          }

          if (type != TRIGGER)
          {
            if (!isDraggedFrameActive && !frameMoveDrag.isActive && ImGui::IsItemActive() &&
                ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
              auto selectedReferences = drag_frame_references_get(frameReference);
              int dragDuration = 0;
              for (auto selectedReference : selectedReferences)
                if (auto selectedFrame = command_frame_get(document, selectedReference))
                  dragDuration += selectedFrame->duration;
              dragDuration = glm::max(1, dragDuration);

              frameMoveDrag = {
                  .type = type,
                  .itemID = id,
                  .animationIndex = reference.animationIndex,
                  .groupType = row.rootGroupType,
                  .groupId = row.rootGroupId,
                  .frameIndex = frameIndex,
                  .duration = dragDuration,
                  .indices = {},
                  .references = {selectedReferences.begin(), selectedReferences.end()},
                  .isActive = true,
              };
            }
          }

          auto rectMin = ImGui::GetItemRectMin();
          auto rectMax = ImGui::GetItemRectMax();
          auto borderColor = isReferenced ? frameBorderColorReferenced : frameBorderColor;
          auto borderThickness = isReferenced ? FRAME_BORDER_THICKNESS_REFERENCED : FRAME_BORDER_THICKNESS;
          drawList->AddRect(rectMin, rectMax, ImGui::GetColorU32(borderColor), FRAME_ROUNDING, 0, borderThickness);

          auto icon = type == TRIGGER ? icon::TRIGGER : INTERPOLATION_ICONS[(int)frame.interpolation];
          auto iconPos = ImVec2(cursorPos.x + (frameTime * frameSize.x),
                                cursorPos.y + (frameSize.y / 2) - (icon_size_get().y / 2));
          ImGui::SetCursorPos(iconPos);
          ImGui::Image(resources.icon_id_get(icon), icon_size_get());
          overlay_icon(resources.icon_id_get(icon), iconTintDefault, false);

          if (type != TRIGGER) frameTime += frame.duration;

          ImGui::PopID();
        }

        if (isFrameMovePreview)
        {
          drawList->AddRectFilled(frameMovePreviewMin, frameMovePreviewMax,
                                  ImGui::GetColorU32(ImGuiCol_DragDropTargetBg), FRAME_ROUNDING);
          drawList->AddRect(frameMovePreviewMin, frameMovePreviewMax, ImGui::GetColorU32(ImGuiCol_DragDropTarget),
                            FRAME_ROUNDING, 0, ImGui::GetStyle().DragDropTargetBorderSize);
        }

        if (isFrameMoveHoveredFrame)
          drawList->AddRect(frameMoveHoveredFrameMin, frameMoveHoveredFrameMax,
                            ImGui::GetColorU32(ImGuiCol_DragDropTarget), FRAME_ROUNDING, 0,
                            ImGui::GetStyle().DragDropTargetBorderSize * 1.5f);

        if (!isFrameBoxSelecting && !frameMoveDrag.isActive && !isDraggedFrameActive && ImGui::IsWindowHovered() &&
            (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right)) &&
            !ImGui::IsAnyItemHovered())
          row_selection_set(row);
      }
    }

    if (isDraggedFrameActive)
    {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
      auto durationDelta =
          draggedFrameWidth > 0.0f
              ? static_cast<int>((ImGui::GetIO().MousePos.x - draggedFrameStartMouseX) / draggedFrameWidth)
              : hoveredTime - draggedFrameStart;
      auto isDraggedFrameChanged = draggedFrameType == TRIGGER ? hoveredTime != draggedFrameStart : durationDelta != 0;

      if (!isDraggedFrameSnapshot && isDraggedFrameChanged)
      {
        isDraggedFrameSnapshot = true;
        edit_begin_push(draggedFrameType == TRIGGER ? EDIT_TRIGGER_AT_FRAME : EDIT_FRAME_DURATION);
      }

      if (isDraggedFrameSnapshot)
      {
        auto targetReference = draggedFrameReference;
        if (draggedFrameType == TRIGGER)
        {
          auto atFrame = glm::clamp(
              hoveredTime, 0, settings.playbackIsClamp && animation ? animation->frameNum - 1 : FRAME_NUM_MAX - 1);
          command_push([=](Manager&, Document& document)
                       { edit::trigger_at_frame_set(document.model, targetReference, atFrame); });
        }
        else
        {
          std::map<Reference, int> durations{};
          if (draggedFrameStartDurations.empty())
            durations[targetReference] = draggedFrameStartDuration + durationDelta;
          for (auto frameDuration : draggedFrameStartDurations)
            durations[frameDuration.reference] = frameDuration.duration + durationDelta;
          command_push([=](Manager&, Document& document) { edit::frame_durations_set(document.model, durations); });
        }
      }

      if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
      {
        auto targetReference = draggedFrameReference;
        auto targetType = draggedFrameType;
        command_push(
            [=, this](Manager&, Document& document)
            {
              auto item = document.model.track_edit(targetReference);
              if (targetType == TRIGGER && item) model::frames_sort_by_at_frame(*item);
              document.change();
            });
        isDraggedFrameActive = false;
        draggedFrameReference = {};
        draggedFrameType = NONE;
        draggedFrameIndex = -1;
        draggedFrameStart = -1;
        draggedFrameStartDuration = -1;
        draggedFrameStartDurations.clear();
        draggedFrameStartMouseX = 0.0f;
        draggedFrameWidth = 0.0f;
        isDraggedFrameSnapshot = false;
      }
    }

    context_menu();

    ImGui::EndChild();
    if (isLightTheme && isDefaultChild) ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    index++;
    ImGui::PopID();
  }

  void TimelineContext::frames_child()
  {
    auto cursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cursorPos.x + ITEM_CHILD_WIDTH, cursorPos.y));

    auto framesChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);

    if (ImGui::BeginChild("##Frames Child", framesChildSize, ImGuiChildFlags_Borders))
    {
      auto viewListChildSize =
          ImVec2(ImGui::GetContentRegionAvail().x,
                 ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - style.ItemSpacing.y * 2);

      auto animationsLength = [&]()
      {
        int length{};
        for (auto [index, item] : model.animations_get())
          length = std::max(length, model::animation_length_get(*item));
        return length;
      }();
      auto childWidth = animationsLength * ImGui::GetTextLineHeight();
      if (animation && animation->frameNum > animationsLength)
        childWidth = animation->frameNum * ImGui::GetTextLineHeight();
      childWidth = std::max(childWidth, ImGui::GetContentRegionAvail().x);

      childWidth *= WIDTH_MULTIPLIER;

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
      if (ImGui::BeginChild("##Frames List Child", viewListChildSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        playheadLineCenterX = 0.0f;
        playheadLineTopY = 0.0f;
        isPlayheadLineSet = false;
        isFrameBoxClipSet = false;
        if (animation && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
        {
          group_selection_reset_for(document);
          document.frame_references_set(document.selected_item_frame_references_get());
        }

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2());
        if (ImGui::BeginTable("##Frames List Table", 1,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
        {
          ImGuiWindow* window = ImGui::GetCurrentWindow();
          window->Flags |= ImGuiWindowFlags_NoScrollWithMouse;

          scroll.x = ImGui::GetScrollX();
          scroll.y = ImGui::GetScrollY();

          isHorizontalScroll = window->ScrollbarX;

          if (isWindowHovered)
          {
            auto& io = ImGui::GetIO();
            auto lineHeight = ImGui::GetTextLineHeightWithSpacing() * 2;

            scroll.x -= io.MouseWheelH * lineHeight;
            scroll.y -= io.MouseWheel * lineHeight;
          }

          ImGui::SetScrollX(scroll.x);
          ImGui::SetScrollY(scroll.y);

          auto frameBoxDrawList = ImGui::GetWindowDrawList();
          frameBoxClipMin = frameBoxDrawList->GetClipRectMin();
          frameBoxClipMax = frameBoxDrawList->GetClipRectMax();
          isFrameBoxClipSet = true;
          if (isFrameBoxPending || isFrameBoxSelecting)
          {
            auto& io = ImGui::GetIO();
            auto threshold = ImGui::GetTextLineHeightWithSpacing() * 0.125f;
            frameBoxEnd = frame_box_content_point_get();
            auto distance = ImVec2(frameBoxEnd.x - frameBoxStart.x, frameBoxEnd.y - frameBoxStart.y);
            if (isFrameBoxPending && distance.x * distance.x + distance.y * distance.y >= threshold * threshold)
            {
              isFrameBoxPending = false;
              isFrameBoxSelecting = true;
            }
            auto edgeSize = ImGui::GetTextLineHeightWithSpacing();
            auto scrollStep = edgeSize * 0.5f;
            if (isFrameBoxSelecting)
            {
              if (io.MousePos.x < frameBoxClipMin.x + edgeSize)
                scroll.x -= scrollStep;
              else if (io.MousePos.x > frameBoxClipMax.x - edgeSize)
                scroll.x += scrollStep;
              if (io.MousePos.y < frameBoxClipMin.y + edgeSize)
                scroll.y -= scrollStep;
              else if (io.MousePos.y > frameBoxClipMax.y - edgeSize)
                scroll.y += scrollStep;
              ImGui::SetScrollX(scroll.x);
              ImGui::SetScrollY(scroll.y);
              frameBoxEnd = frame_box_content_point_get();
              frameBoxSelection.clear();
            }
          }

          int index{};

          ImGui::TableSetupScrollFreeze(0, 1);
          ImGui::TableSetupColumn("##Frames");

          auto frames_child_row = [&](const TimelineItemRow& row)
          {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            frame_child(row, index, childWidth);
          };

          frames_child_row({.type = NONE});

          if (animation)
          {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

            for (const auto& row : timeline_item_rows_get())
              frames_child_row(row);

            ImGui::PopStyleVar();
          }
          ImGui::EndTable();
        }

        if (isFrameBoxClipSet)
        {
          auto overlayCursor = ImGui::GetCursorScreenPos();
          auto overlayPos = ImGui::GetWindowPos();
          auto overlaySize = ImGui::GetWindowSize();
          ImGui::SetCursorScreenPos(overlayPos);
          if (ImGui::BeginChild("##Frames Overlay Child", overlaySize, ImGuiChildFlags_None,
                                ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground |
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                    ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs))
            frame_overlay_draw(ImGui::GetWindowDrawList(), frameBoxClipMin, frameBoxClipMax);
          ImGui::EndChild();
          ImGui::SetCursorScreenPos(overlayCursor);
        }

        if (isFrameBoxPending || isFrameBoxSelecting)
        {
          if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
          {
            if (isFrameBoxSelecting)
            {
              group_selection_reset_for(document);
              if (isFrameBoxAdditive)
              {
                auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
                selectedFrames.insert(frameBoxSelection.begin(), frameBoxSelection.end());
                document.frame_references_set(std::move(selectedFrames));
              }
              else
                document.frame_references_set(frameBoxSelection);
            }
            isFrameBoxPending = false;
            isFrameBoxSelecting = false;
            frameBoxSelection.clear();
          }
        }

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      ImGui::SetCursorPos(
          ImVec2(ImGui::GetStyle().WindowPadding.x, ImGui::GetCursorPos().y + ImGui::GetStyle().ItemSpacing.y));

      auto widgetSize = widget_size_with_row_get(10);

      ImGui::BeginDisabled(!animation);
      {
        auto label = playback.isPlaying ? localize.get(LABEL_PAUSE) : localize.get(LABEL_PLAY);
        auto tooltip =
            playback.isPlaying ? localize.get(TOOLTIP_PAUSE_ANIMATION) : localize.get(TOOLTIP_PLAY_ANIMATION);

        shortcut(manager.chords[SHORTCUT_PLAY_PAUSE]);
        if (ImGui::Button(label, widgetSize)) playback.toggle();
        set_item_tooltip_shortcut(tooltip, settings.shortcutPlayPause);

        ImGui::SameLine();

        auto item = selected_item_get();
        auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
        auto selectedBakeFrames = selectedFrames;
        std::erase_if(selectedBakeFrames, is_trigger_reference);

        ImGui::BeginDisabled(!item);
        {
          shortcut(manager.chords[SHORTCUT_INSERT_FRAME]);
          if (ImGui::Button(localize.get(LABEL_INSERT), widgetSize)) frame_insert();
          set_item_tooltip_shortcut(localize.get(TOOLTIP_INSERT_FRAME), settings.shortcutInsertFrame);

          ImGui::SameLine();

          ImGui::BeginDisabled(selectedFrames.empty());
          {
            shortcut(manager.chords[SHORTCUT_REMOVE]);
            if (ImGui::Button(localize.get(LABEL_DELETE), widgetSize)) frames_delete_action();
            set_item_tooltip_shortcut(localize.get(TOOLTIP_DELETE_FRAMES), settings.shortcutRemove);

            ImGui::SameLine();

            ImGui::BeginDisabled(selectedBakeFrames.empty());
            if (ImGui::Button(localize.get(LABEL_BAKE), widgetSize)) bakePopup.open();
            set_item_tooltip_shortcut(localize.get(TOOLTIP_BAKE_FRAMES), settings.shortcutBake);
            ImGui::EndDisabled();
          }
          ImGui::EndDisabled();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!animation || animation->frameNum == animation_length_get(*animation));
        shortcut(manager.chords[SHORTCUT_FIT]);
        if (ImGui::Button(localize.get(LABEL_FIT_ANIMATION_LENGTH), widgetSize)) fit_animation_length();
        set_item_tooltip_shortcut(localize.get(TOOLTIP_FIT_ANIMATION_LENGTH), settings.shortcutFit);
        ImGui::EndDisabled();

        ImGui::SameLine();

        auto frameNum = animation ? animation->frameNum : dummy_value<int>();
        auto currentAnimationIndex = reference.animationIndex;
        ImGui::SetNextItemWidth(widgetSize.x);
        auto isLengthChanged =
            input_int_range(localize.get(LABEL_ANIMATION_LENGTH), frameNum, FRAME_NUM_MIN, FRAME_NUM_MAX, STEP,
                            STEP_FAST, !animation ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0);
        if (ImGui::IsItemActivated()) animationLengthEditIndex = currentAnimationIndex;
        if (isLengthChanged && animation)
        {
          auto animationIndex = animationLengthEditIndex != -1 ? animationLengthEditIndex : currentAnimationIndex;
          edit_push(EDIT_ANIMATION_LENGTH,
                    [=](model::Model& model)
                    {
                      if (auto animation = model.animation_edit(animationIndex)) animation->frameNum = frameNum;
                    });
        }
        if (ImGui::IsItemDeactivated()) animationLengthEditIndex = -1;
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ANIMATION_LENGTH));

        ImGui::SameLine();

        auto isLoop = animation ? animation->isLoop : dummy_value<bool>();
        ImGui::SetNextItemWidth(widgetSize.x);
        if (ImGui::Checkbox(localize.get(LABEL_LOOP), &isLoop) && animation)
        {
          auto animationIndex = reference.animationIndex;
          edit_push(EDIT_LOOP,
                    [=](model::Model& model)
                    {
                      if (auto animation = model.animation_edit(animationIndex)) animation->isLoop = isLoop;
                    });
        }
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LOOP_ANIMATION));
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      auto fps = model.info.fps;
      ImGui::SetNextItemWidth(widgetSize.x);
      if (input_int_range(localize.get(LABEL_FPS), fps, FPS_MIN, FPS_MAX))
      {
        edit_push(EDIT_FPS, [=](model::Model& model) { model.info.fps = fps; });
      }
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FPS));

      ImGui::SameLine();

      auto createdBy = model.info.createdBy;
      ImGui::SetNextItemWidth(widgetSize.x);
      if (input_text_string(localize.get(LABEL_AUTHOR), &createdBy))
      {
        edit_push(EDIT_AUTHOR, [=](model::Model& model) { model.info.createdBy = createdBy; });
      }
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AUTHOR));

      ImGui::SameLine();

      ImGui::SetNextItemWidth(widgetSize.x);
      ImGui::Checkbox(localize.get(LABEL_SOUND), &settings.timelineIsSound);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SOUND));

      ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    ImGui::SetCursorPos(cursorPos);
  }
}
