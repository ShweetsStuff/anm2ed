#include "context.hpp"

namespace anm2ed::imgui
{
  void TimelineContext::item_child(const TimelineItemRow& row, int index)
  {
    ImGui::PushID(index);

    auto type = row.type;
    auto id = row.id;
    auto row_reference_make = [&]()
    { return Reference{reference.animationIndex, type, id, -1, row.rootGroupType, row.rootGroupId}; };
    if (row.isGroup)
    {
      auto group = row_group_get(row);
      if (!group)
      {
        ImGui::PopID();
        return;
      }

      auto label = group->name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : group->name;
      auto itemSize = ImVec2(ImGui::GetContentRegionAvail().x, rowFrameChildHeight);
      auto isGroupVisible = group->isVisible;
      auto colorVec = color_get(COLOR_ITEM_BASE, type);
      if (is_group_selected(row)) colorVec = color_get(COLOR_ITEM_SELECTED, type);
      auto color = to_imvec4(isGroupVisible ? colorVec : colorVec * COLOR_HIDDEN_MULTIPLIER);
      ImGui::PushStyleColor(ImGuiCol_ChildBg, color);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      if (ImGui::BeginChild("##Group Child", itemSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse))
      {
        auto cursorPos = ImGui::GetCursorPos();
        auto groupChildMin = ImGui::GetWindowPos();
        auto groupChildMax =
            ImVec2(groupChildMin.x + ImGui::GetWindowSize().x, groupChildMin.y + ImGui::GetWindowSize().y);

        auto toggle_group = [&]()
        {
          auto targetRow = row;
          auto targetAnimationIndex = reference.animationIndex;
          edit_command_push(EDIT_TOGGLE_GROUP_EXPANDED, Document::ITEMS,
                            [=, this](Manager&, Document& document) mutable
                            {
                              auto animation = document.anm2.element_get(ElementType::ANIMATION, targetAnimationIndex);
                              auto container =
                                  animation ? child_first_get(*animation, TYPE_CONTAINERS[targetRow.type]) : nullptr;
                              auto group =
                                  container ? child_id_get(*container, ElementType::GROUP, targetRow.id) : nullptr;
                              if (!group) return;
                              group->isExpanded = !group->isExpanded;
                            });
        };

        ImGui::SetCursorPos(to_imvec2(to_vec2(cursorPos) - to_vec2(style.ItemSpacing)));
        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("##Group Button", itemSize);
        auto groupButtonMin = ImGui::GetItemRectMin();
        auto groupButtonMax = ImGui::GetItemRectMax();
        auto mousePos = ImGui::GetIO().MousePos;
        auto is_mouse_in_rect = [](ImVec2 mouse, ImVec2 min, ImVec2 max)
        { return mouse.x >= min.x && mouse.x < max.x && mouse.y >= min.y && mouse.y < max.y; };
        auto isGroupHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                              is_mouse_in_rect(mousePos, groupButtonMin, groupButtonMax);
        bool isGroupTooltipDelayed{};
        if (isGroupHovered)
        {
          auto& imguiStyle = ImGui::GetStyle();
          auto previousTooltipDelay = imguiStyle.HoverDelayNormal;
          imguiStyle.HoverDelayNormal = FRAME_TOOLTIP_HOVER_DELAY;
          isGroupTooltipDelayed =
              ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal |
                                   ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_NoSharedDelay);
          imguiStyle.HoverDelayNormal = previousTooltipDelay;
        }

        if (ImGui::BeginDragDropSource(DRAG_DROP_SOURCE_FLAGS))
        {
          rowDragReferences = row_drag_references_get(row);
          ImGui::SetDragDropPayload("Timeline Row Drag Drop", rowDragReferences.data(),
                                    (int)rowDragReferences.size() * (int)sizeof(TimelineRowReference));
          ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
          if (auto payload = ImGui::AcceptDragDropPayload("Timeline Row Drag Drop",
                                                          ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                              ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
          {
            auto groupDropThird = (groupButtonMax.y - groupButtonMin.y) / 3.0f;
            auto dropZone = DropZone::BEFORE;
            if (mousePos.y >= groupButtonMax.y - groupDropThird)
              dropZone = DropZone::AFTER;
            else if (mousePos.y >= groupButtonMin.y + groupDropThird)
              dropZone = DropZone::INSIDE;
            auto isDropAfter = dropZone != DropZone::BEFORE;
            auto payloadRows = (TimelineRowReference*)payload->Data;
            auto payloadCount = payload->DataSize / sizeof(TimelineRowReference);
            std::vector<TimelineRowReference> draggedRows(payloadRows, payloadRows + payloadCount);
            auto isDropIntoGroup = dropZone == DropZone::INSIDE && (row.type == LAYER || row.type == NULL_);
            for (auto draggedRow : draggedRows)
              if (draggedRow.isGroup || draggedRow.type != row.type) isDropIntoGroup = false;

            if (isDropIntoGroup)
              drop_box_draw(ImGui::GetWindowDrawList(), groupChildMin, groupChildMax);
            else
              drop_line_draw(ImGui::GetWindowDrawList(), groupChildMin, groupChildMax, isDropAfter);

            if (payload->IsDelivery()) rows_move_to_row(draggedRows, row, isDropAfter, isDropIntoGroup);
          }
          ImGui::EndDragDropTarget();
        }

        ImGui::SetCursorPos(cursorPos);
        auto folderIcon = group->isExpanded ? icon::FOLDER_OPEN : icon::FOLDER;
        ImGui::Image(resources.icon_id_get(folderIcon), icon_size_get());
        auto iconMin = ImGui::GetItemRectMin();
        auto iconMax = ImGui::GetItemRectMax();
        overlay_icon(resources.icon_id_get(folderIcon), itemIconTint, false);
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, itemTextColor);
        ImGui::TextUnformatted(label.c_str());
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

        ImGui::SetCursorPos(ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                                   (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
        int visibleIcon = isGroupVisible ? icon::VISIBLE : icon::INVISIBLE;
        if (ImGui::ImageButton("##Group Visible Toggle", resources.icon_id_get(visibleIcon), icon_size_get()))
        {
          auto targetAnimationIndex = reference.animationIndex;
          auto targetType = type;
          auto targetGroupId = group->id;
          auto targetVisible = !isGroupVisible;
          edit_command_push(EDIT_TOGGLE_ITEM_VISIBILITY, Document::FRAMES,
                            [=, this](Manager&, Document& document)
                            {
                              auto animation = document.anm2.element_get(ElementType::ANIMATION, targetAnimationIndex);
                              auto container =
                                  animation ? child_first_get(*animation, TYPE_CONTAINERS[targetType]) : nullptr;
                              if (!container) return;
                              auto group = child_id_get(*container, ElementType::GROUP, targetGroupId);
                              if (!group) return;
                              group->isVisible = targetVisible;
                            });
        }
        auto visibleButtonMin = ImGui::GetItemRectMin();
        auto visibleButtonMax = ImGui::GetItemRectMax();
        overlay_icon(resources.icon_id_get(visibleIcon), itemIconTint, false);
        ImGui::SetItemTooltip("%s", isGroupVisible ? localize.get(TOOLTIP_ITEM_VISIBILITY_SHOWN)
                                                   : localize.get(TOOLTIP_ITEM_VISIBILITY_HIDDEN));
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        auto isIconHovered = isGroupHovered && is_mouse_in_rect(mousePos, iconMin, iconMax);
        auto isVisibleButtonHovered = isGroupHovered && is_mouse_in_rect(mousePos, visibleButtonMin, visibleButtonMax);
        if (isGroupTooltipDelayed && !isVisibleButtonHovered)
        {
          ImGui::BeginTooltip();
          ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
          ImGui::TextUnformatted(label.c_str());
          ImGui::PopFont();
          ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(group->id)).c_str());
          auto groupItemsCount = group_items_count_get(type, group->id);
          ImGui::TextUnformatted(
              std::vformat(localize.get(FORMAT_ITEMS_COUNT), std::make_format_args(groupItemsCount)).c_str());
          ImGui::EndTooltip();
        }
        if (isIconHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
            !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left))
          toggle_group();
        else if (isGroupHovered && !isIconHovered && !isVisibleButtonHovered &&
                 ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
          group_properties_open(row, *group);
        else if (isGroupHovered && !isIconHovered && !isVisibleButtonHovered &&
                 ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                 !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left))
          row_selection_set(row);
      }
      ImGui::EndChild();
      ImGui::PopStyleVar(2);
      ImGui::PopStyleColor();
      ImGui::PopID();
      return;
    }

    auto item = item_get(type, id, row.rootGroupType, row.rootGroupId);
    if (type != NONE && !item)
    {
      ImGui::PopID();
      return;
    }
    auto isItemVisible = item ? item->isVisible : false;
    auto isVisible = item ? item->isVisible && is_track_group_visible(type, row.groupId) : false;
    if (item && type == ROOT && row.rootGroupId != -1)
      isVisible = item->isVisible && is_track_group_visible(row.rootGroupType, row.rootGroupId);
    auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
    if (isOnlyShowLayers && type != LAYER) isVisible = false;
    auto isReferenced = is_same_item(reference, row_reference_make());
    auto isItemSelected = is_row_selected(row);

    auto label = [&]() -> std::string
    {
      if (type == LAYER)
      {
        auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, id);
        if (!layer) return localize.get(TYPE_STRINGS[type]);
        return std::vformat(localize.get(FORMAT_LAYER), std::make_format_args(id, layer->name, layer->spritesheetId));
      }
      if (type == NULL_)
      {
        auto null = anm2.element_get(ElementType::NULL_ELEMENT, id);
        if (!null) return localize.get(TYPE_STRINGS[type]);
        return std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, null->name));
      }
      return localize.get(TYPE_STRINGS[type]);
    }();
    auto icon = TYPE_ICONS[type];
    auto iconTintCurrent = isLightTheme && type == NONE ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : itemIconTint;
    auto colorVec = color_get(isItemSelected && type != NONE ? COLOR_ITEM_SELECTED : COLOR_ITEM_BASE, type);
    auto color = to_imvec4(colorVec);
    color = !isVisible ? to_imvec4(colorVec * COLOR_HIDDEN_MULTIPLIER) : color;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, color);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

    auto itemSize = ImVec2(ImGui::GetContentRegionAvail().x, rowFrameChildHeight);

    if (ImGui::BeginChild(label.c_str(), itemSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse))
    {
      auto cursorPos = ImGui::GetCursorPos();
      auto itemChildMin = ImGui::GetWindowPos();
      auto itemChildMax = ImVec2(itemChildMin.x + ImGui::GetWindowSize().x, itemChildMin.y + ImGui::GetWindowSize().y);

      if (type != NONE)
      {
        ImGui::SetCursorPos(to_imvec2(to_vec2(cursorPos) - to_vec2(style.ItemSpacing)));
        ImGui::SetNextItemAllowOverlap();
        ImGui::SetNextItemStorageID(id);
        ImGui::InvisibleButton("##Item Button", itemSize);
        auto itemButtonMin = ImGui::GetItemRectMin();
        auto itemButtonMax = ImGui::GetItemRectMax();
        auto isItemButtonHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        if (type == LAYER || type == NULL_)
        {
          if (ImGui::BeginDragDropSource(DRAG_DROP_SOURCE_FLAGS))
          {
            rowDragReferences = row_drag_references_get(row);
            ImGui::SetDragDropPayload("Timeline Row Drag Drop", rowDragReferences.data(),
                                      (int)rowDragReferences.size() * (int)sizeof(TimelineRowReference));
            ImGui::EndDragDropSource();
          }

          if (ImGui::BeginDragDropTarget())
          {
            if (auto payload = ImGui::AcceptDragDropPayload("Timeline Row Drag Drop",
                                                            ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                                ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
              auto isDropAfter = is_drop_after(itemButtonMin, itemButtonMax);
              drop_line_draw(ImGui::GetWindowDrawList(), itemChildMin, itemChildMax, isDropAfter);

              auto payloadRows = (TimelineRowReference*)payload->Data;
              auto payloadCount = payload->DataSize / sizeof(TimelineRowReference);
              std::vector<TimelineRowReference> draggedRows(payloadRows, payloadRows + payloadCount);
              if (payload->IsDelivery()) rows_move_to_row(draggedRows, row, isDropAfter, false);
            }
            ImGui::EndDragDropTarget();
          }
        }

        if (isItemButtonHovered)
        {
          if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            item_base_properties_open(type, id);
          else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                   !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left))
            row_selection_set(row);

          auto& imguiStyle = ImGui::GetStyle();
          auto previousTooltipFlags = imguiStyle.HoverFlagsForTooltipMouse;
          auto previousTooltipDelay = imguiStyle.HoverDelayNormal;
          imguiStyle.HoverFlagsForTooltipMouse = ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal |
                                                 ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_NoSharedDelay;
          imguiStyle.HoverDelayNormal = FRAME_TOOLTIP_HOVER_DELAY;
          bool showItemTooltip = ImGui::BeginItemTooltip();
          imguiStyle.HoverFlagsForTooltipMouse = previousTooltipFlags;
          imguiStyle.HoverDelayNormal = previousTooltipDelay;

          if (showItemTooltip)
          {
            auto yesNoLabel = [&](bool value) { return value ? localize.get(BASIC_YES) : localize.get(BASIC_NO); };
            auto visibleLabel = yesNoLabel(isVisible);
            auto framesCount = item ? track_frames_count_get(*item) : 0;

            switch (type)
            {
              case ROOT:
              {
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(localize.get(BASIC_ROOT));
                ImGui::PopFont();

                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                auto transformLabel = yesNoLabel(settings.previewIsRootTransform);
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_TRANSFORM), std::make_format_args(transformLabel)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_FRAMES_COUNT), std::make_format_args(framesCount)).c_str());
                break;
              }
              case LAYER:
              {
                auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, id);
                if (!layer) break;
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(layer->name.c_str());
                ImGui::PopFont();

                ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_SPRITESHEET_ID), std::make_format_args(layer->spritesheetId))
                        .c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_FRAMES_COUNT), std::make_format_args(framesCount)).c_str());
                break;
              }
              case NULL_:
              {
                auto nullInfo = anm2.element_get(ElementType::NULL_ELEMENT, id);
                if (!nullInfo) break;
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(nullInfo->name.c_str());
                ImGui::PopFont();

                auto rectLabel = yesNoLabel(nullInfo->isShowRect);
                ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_RECT), std::make_format_args(rectLabel)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_FRAMES_COUNT), std::make_format_args(framesCount)).c_str());
                break;
              }
              case TRIGGER:
              {
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(localize.get(BASIC_TRIGGERS));
                ImGui::PopFont();

                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_TRIGGERS_COUNT), std::make_format_args(framesCount)).c_str());
                break;
              }
              default:
                break;
            }

            ImGui::EndTooltip();
          }
        }

        auto contentCursorPos = cursorPos;
        contentCursorPos.x += (float)row.depth * ImGui::GetTextLineHeightWithSpacing();
        ImGui::SetCursorPos(contentCursorPos);

        ImGui::Image(resources.icon_id_get(icon), icon_size_get());
        overlay_icon(resources.icon_id_get(icon), iconTintCurrent, false);
        ImGui::SameLine();
        if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
        ImGui::PushStyleColor(ImGuiCol_Text, itemTextColor);
        ImGui::TextUnformatted(label.c_str());
        ImGui::PopStyleColor();
        if (isReferenced) ImGui::PopFont();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

        ImGui::SetCursorPos(ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                                   (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
        int visibleIcon = isItemVisible ? icon::VISIBLE : icon::INVISIBLE;
        if (ImGui::ImageButton("##Visible Toggle", resources.icon_id_get(visibleIcon), icon_size_get()))
        {
          auto animationIndex = reference.animationIndex;
          auto targetType = type;
          auto targetID = id;
          auto targetGroupType = row.rootGroupType;
          auto targetGroupId = row.rootGroupId;
          edit_command_push(EDIT_TOGGLE_ITEM_VISIBILITY, Document::FRAMES,
                            [=, this](Manager&, Document& document)
                            {
                              auto item = command_item_get(document, animationIndex, targetType, targetID,
                                                           targetGroupType, targetGroupId);
                              if (!item) return;
                              item->isVisible = !item->isVisible;
                            });
        }
        overlay_icon(resources.icon_id_get(visibleIcon), iconTintCurrent, false);
        ImGui::SetItemTooltip("%s", isItemVisible ? localize.get(TOOLTIP_ITEM_VISIBILITY_SHOWN)
                                                  : localize.get(TOOLTIP_ITEM_VISIBILITY_HIDDEN));

        if (type == NULL_)
        {
          if (auto null = anm2.element_get(ElementType::NULL_ELEMENT, id))
          {
            auto& isShowRect = null->isShowRect;
            auto rectIcon = isShowRect ? icon::SHOW_RECT : icon::HIDE_RECT;
            ImGui::SetCursorPos(
                ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                       (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
            if (ImGui::ImageButton("##Rect Toggle", resources.icon_id_get(rectIcon), icon_size_get()))
            {
              auto nullID = id;
              edit_command_push(EDIT_TOGGLE_NULL_RECT, Document::FRAMES,
                                [=, this](Manager&, Document& document)
                                {
                                  auto nulls = document.anm2.element_get(ElementType::NULLS);
                                  auto null = nulls ? child_id_get(*nulls, ElementType::NULL_ELEMENT, nullID) : nullptr;
                                  if (!null) return;
                                  null->isShowRect = !null->isShowRect;
                                });
            }
            overlay_icon(resources.icon_id_get(rectIcon), iconTintCurrent, false);
            ImGui::SetItemTooltip("%s", isShowRect ? localize.get(TOOLTIP_NULL_RECT_SHOWN)
                                                   : localize.get(TOOLTIP_NULL_RECT_HIDDEN));
          }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
      }
      else
      {
        auto cursorPos = ImGui::GetCursorPos();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::SetCursorPos(ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                                   (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));

        auto& isShowUnused = settings.timelineIsShowUnused;
        auto unusedIcon = isShowUnused ? icon::SHOW_UNUSED : icon::HIDE_UNUSED;
        if (ImGui::ImageButton("##Unused Toggle", resources.icon_id_get(unusedIcon), icon_size_get()))
          isShowUnused = !isShowUnused;
        overlay_icon(resources.icon_id_get(unusedIcon), iconTintCurrent, false);
        ImGui::SetItemTooltip("%s", isShowUnused ? localize.get(TOOLTIP_UNUSED_ITEMS_SHOWN)
                                                 : localize.get(TOOLTIP_UNUSED_ITEMS_HIDDEN));

        auto& showLayersOnly = settings.timelineIsOnlyShowLayers;
        auto layersIcon = showLayersOnly ? icon::SHOW_LAYERS : icon::HIDE_LAYERS;
        ImGui::SetCursorPos(
            ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                   (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
        if (ImGui::ImageButton("##Layers Toggle", resources.icon_id_get(layersIcon), icon_size_get()))
          showLayersOnly = !showLayersOnly;
        overlay_icon(resources.icon_id_get(layersIcon), iconTintCurrent, false);
        ImGui::SetItemTooltip("%s", showLayersOnly ? localize.get(TOOLTIP_ONLY_LAYERS_VISIBLE)
                                                   : localize.get(TOOLTIP_ALL_ITEMS_VISIBLE));
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        ImGui::SetCursorPos(cursorPos);

        ImGui::BeginDisabled();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("(?)");
        ImGui::PopStyleColor();
        auto tooltipShortcuts =
            std::vformat(localize.get(TOOLTIP_TIMELINE_SHORTCUTS),
                         std::make_format_args(settings.shortcutMovePlayheadBack, settings.shortcutMovePlayheadForward,
                                               settings.shortcutShortenFrame, settings.shortcutExtendFrame,
                                               settings.shortcutPreviousFrame, settings.shortcutNextFrame,
                                               settings.shortcutPreviousItem, settings.shortcutNextItem));
        ImGui::SetItemTooltip("%s", tooltipShortcuts.c_str());
        ImGui::EndDisabled();
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImGui::PopID();
  }

  void TimelineContext::items_child()
  {
    auto itemsChildSize = ImVec2(ITEM_CHILD_WIDTH, ImGui::GetContentRegionAvail().y);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    bool isItemsChildOpen = ImGui::BeginChild("##Items Child", itemsChildSize, ImGuiChildFlags_Borders);
    ImGui::PopStyleVar();
    if (isItemsChildOpen)
    {
      auto itemsListChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y -
                                                                             ImGui::GetTextLineHeightWithSpacing() -
                                                                             ImGui::GetStyle().ItemSpacing.y * 2);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
      if (ImGui::BeginChild("##Items List Child", itemsListChildSize, ImGuiChildFlags_Borders,
                            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
      {
        if (animation && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
        {
          auto rowReferences = timeline_row_references_get();
          row_selection_clear();
          for (auto rowReference : rowReferences)
            row_selection_insert(rowReference);
          if (!rowReferences.empty())
          {
            rowSelectionAnchor = rowReferences.front();
            isRowSelectionAnchorSet = true;
          }
          frames_selection_reset_for(document);
        }

        if (animation && shortcut(manager.chords[SHORTCUT_GROUP], shortcut::FOCUSED) &&
            !item_references_groupable_get().empty())
          item_group();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2());
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
        if (ImGui::BeginTable("##Item Table", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
        {
          ImGui::GetCurrentWindow()->Flags |= ImGuiWindowFlags_NoScrollWithMouse;
          ImGui::SetScrollY(scroll.y);

          ImGui::TableSetupScrollFreeze(0, 1);
          ImGui::TableSetupColumn("##Items");

          auto item_child_row = [&](const TimelineItemRow& row, int index)
          {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            item_child(row, index);
          };

          int index{};
          item_child_row({.type = NONE}, index++);
          if (animation)
            for (const auto& row : timeline_item_rows_get())
              item_child_row(row, index++);

          if (isHorizontalScroll && ImGui::GetCurrentWindow()->ScrollbarY)
          {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Dummy(ImVec2(0, style.ScrollbarSize));
          }

          ImGui::EndTable();
        }
        ImGui::PopStyleVar(2);

        item_context_menu();
      }
      ImGui::PopStyleVar(2);
      ImGui::EndChild();

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + style.WindowPadding.x, ImGui::GetCursorPosY()));
      auto widgetSize = widget_size_with_row_get(2, ImGui::GetContentRegionAvail().x - style.WindowPadding.x);

      ImGui::BeginDisabled(!animation);
      {
        shortcut(manager.chords[SHORTCUT_ADD]);
        if (ImGui::Button(localize.get(BASIC_ADD), widgetSize)) itemProperties.open();
        set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_ITEM), settings.shortcutAdd);
        ImGui::SameLine();

        auto selectedRows = selected_row_references_get();
        auto isRemoveAvailable = std::ranges::any_of(selectedRows, [](const TimelineRowReference& row)
                                                     { return row.type == LAYER || row.type == NULL_; });
        ImGui::BeginDisabled(!isRemoveAvailable);
        shortcut(manager.chords[SHORTCUT_REMOVE]);
        if (ImGui::Button(localize.get(BASIC_REMOVE), widgetSize)) item_remove();
        set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_ITEMS), settings.shortcutRemove);
        ImGui::EndDisabled();
      }
      ImGui::EndDisabled();

      ImGui::PopStyleVar();
    }
    ImGui::EndChild();
  }
}
