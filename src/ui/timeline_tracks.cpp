#include "ui/timeline_context.h"
#include "ui/layout.h"

void TimelineContext::item_child(const TimelineRow& row, int index)
{
    ImGui::PushID(index);

    auto type = row.type;
    auto id = row.id;
    auto row_reference_make = [&]() {
        return Reference{reference.animationIndex, type, id, -1, row.groupType, row.groupId};
    };
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
        if (is_row_reference_selected(row))
            colorVec = color_get(COLOR_ITEM_SELECTED, type);
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

            auto toggle_group = [&]() {
                auto targetRow = row;
                auto targetAnimationIndex = reference.animationIndex;
                edit_push(EDIT_TOGGLE_GROUP_EXPANDED, [=](Model& model) {
                    if (auto group = model.track_group_edit(targetAnimationIndex, targetRow.type, targetRow.id))
                        group->isExpanded = !group->isExpanded;
                });
            };

            ImGui::SetCursorPos(to_imvec2(to_vec2(cursorPos) - to_vec2(style.ItemSpacing)));
            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("##Group Button", itemSize);
            auto groupButtonMin = ImGui::GetItemRectMin();
            auto groupButtonMax = ImGui::GetItemRectMax();
            auto isGroupHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                                  is_mouse_in(groupButtonMin, groupButtonMax);
            bool isGroupTooltipDelayed = false;
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

            row_drag_drop(row, groupButtonMin, groupButtonMax, groupChildMin, groupChildMax);

            ImGui::SetCursorPos(cursorPos);
            auto folderIcon = group->isExpanded ? ICON_FOLDER_OPEN : ICON_FOLDER;
            ImGui::Image(resources.icon_id_get(folderIcon), icon_size_get());
            auto iconMin = ImGui::GetItemRectMin();
            auto iconMax = ImGui::GetItemRectMax();
            overlay_icon(resources.icon_id_get(folderIcon), itemIconTint);
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Text, itemTextColor);
            ImGui::TextUnformatted(label.c_str());
            ImGui::PopStyleColor();

            if (row_toggle_draw(itemSize, 1,
                                {"##Group Visible Toggle", isGroupVisible, ICON_VISIBLE, ICON_INVISIBLE,
                                 TOOLTIP_ITEM_VISIBILITY_SHOWN, TOOLTIP_ITEM_VISIBILITY_HIDDEN},
                                itemIconTint))
            {
                auto targetAnimationIndex = reference.animationIndex;
                auto targetType = type;
                auto targetGroupId = group->id;
                auto targetVisible = !isGroupVisible;
                edit_push(EDIT_TOGGLE_ITEM_VISIBILITY, [=](Model& model) {
                    if (auto group = model.track_group_edit(targetAnimationIndex, targetType, targetGroupId))
                        group->isVisible = targetVisible;
                });
            }
            auto visibleButtonMin = ImGui::GetItemRectMin();
            auto visibleButtonMax = ImGui::GetItemRectMax();

            auto isIconHovered = isGroupHovered && is_mouse_in(iconMin, iconMax);
            auto isVisibleButtonHovered = isGroupHovered && is_mouse_in(visibleButtonMin, visibleButtonMax);
            if (isGroupTooltipDelayed && !isVisibleButtonHovered)
            {
                ImGui::BeginTooltip();
                text_bold_draw(resources.fonts[FONT_BOLD], label.c_str());
                ImGui::TextUnformatted(localize_format(FORMAT_ID, group->id).c_str());
                auto groupItemsCount = (int)group->items.size();
                ImGui::TextUnformatted(localize_format(FORMAT_ITEMS_COUNT, groupItemsCount).c_str());
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

    auto item = item_get(type, id, row.groupType, row.groupId);
    if (type != NONE && !item)
    {
        ImGui::PopID();
        return;
    }
    auto isItemVisible = item ? item->isVisible : false;
    auto isVisible = is_row_visible(row, item);
    auto isReferenced = is_same_item(reference, row_reference_make());
    auto isItemSelected = is_row_selected(row);

    auto label = [&]() -> std::string {
        if (type == LAYER)
        {
            auto layer = ::item_get(model.content.layers, id);
            if (!layer)
                return localize.get(ITEM_TYPE_STYLES[type].label);
            return layer_label_get(*layer);
        }
        if (type == NULL_)
        {
            auto null = ::item_get(model.content.nulls, id);
            if (!null)
                return localize.get(ITEM_TYPE_STYLES[type].label);
            return localize_format(FORMAT_NULL, id, null->name);
        }
        return localize.get(ITEM_TYPE_STYLES[type].label);
    }();
    auto icon = ITEM_TYPE_STYLES[type].icon;
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
        auto itemChildMax =
            ImVec2(itemChildMin.x + ImGui::GetWindowSize().x, itemChildMin.y + ImGui::GetWindowSize().y);

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
                row_drag_drop(row, itemButtonMin, itemButtonMax, itemChildMin, itemChildMax);

            auto animationLayer = type == LAYER ? ::item_get(model.content.layers, id) : nullptr;
            if (animationLayer && !animationLayer->is_animation())
                animationLayer = nullptr;
            if (isItemButtonHovered)
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    animationLayer ? child_step_in(row) : item_base_properties_open(type, id);
                else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                         !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left))
                    row_selection_set(row);

                auto& imguiStyle = ImGui::GetStyle();
                auto previousTooltipFlags = imguiStyle.HoverFlagsForTooltipMouse;
                auto previousTooltipDelay = imguiStyle.HoverDelayNormal;
                imguiStyle.HoverFlagsForTooltipMouse = ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal |
                                                       ImGuiHoveredFlags_AllowWhenDisabled |
                                                       ImGuiHoveredFlags_NoSharedDelay;
                imguiStyle.HoverDelayNormal = FRAME_TOOLTIP_HOVER_DELAY;
                bool showItemTooltip = ImGui::BeginItemTooltip();
                imguiStyle.HoverFlagsForTooltipMouse = previousTooltipFlags;
                imguiStyle.HoverDelayNormal = previousTooltipDelay;

                if (showItemTooltip)
                {
                    auto yesNoLabel = [&](bool value) {
                        return value ? localize.get(BASIC_YES) : localize.get(BASIC_NO);
                    };
                    auto visibleLabel = yesNoLabel(isVisible);
                    auto framesCount = item ? (int)item->frames.size() : 0;

                    switch (type)
                    {
                        case ROOT:
                        {
                            text_bold_draw(resources.fonts[FONT_BOLD], localize.get(BASIC_ROOT));

                            ImGui::TextUnformatted(localize_format(FORMAT_VISIBLE, visibleLabel).c_str());
                            auto transformLabel = yesNoLabel(settings.previewIsRootTransform);
                            ImGui::TextUnformatted(localize_format(FORMAT_TRANSFORM, transformLabel).c_str());
                            ImGui::TextUnformatted(localize_format(FORMAT_FRAMES_COUNT, framesCount).c_str());
                            break;
                        }
                        case LAYER:
                        {
                            auto layer = ::item_get(model.content.layers, id);
                            if (!layer)
                                break;
                            text_bold_draw(resources.fonts[FONT_BOLD], layer->name.c_str());

                            ImGui::TextUnformatted(localize_format(FORMAT_ID, id).c_str());
                            layer_source_tooltip_draw(document, *layer);
                            ImGui::TextUnformatted(localize_format(FORMAT_VISIBLE, visibleLabel).c_str());
                            ImGui::TextUnformatted(localize_format(FORMAT_FRAMES_COUNT, framesCount).c_str());
                            break;
                        }
                        case NULL_:
                        {
                            auto nullInfo = ::item_get(model.content.nulls, id);
                            if (!nullInfo)
                                break;
                            text_bold_draw(resources.fonts[FONT_BOLD], nullInfo->name.c_str());

                            auto rectLabel = yesNoLabel(nullInfo->isShowRect);
                            ImGui::TextUnformatted(localize_format(FORMAT_ID, id).c_str());
                            ImGui::TextUnformatted(localize_format(FORMAT_RECT, rectLabel).c_str());
                            ImGui::TextUnformatted(localize_format(FORMAT_VISIBLE, visibleLabel).c_str());
                            ImGui::TextUnformatted(localize_format(FORMAT_FRAMES_COUNT, framesCount).c_str());
                            break;
                        }
                        case TRIGGER:
                        {
                            text_bold_draw(resources.fonts[FONT_BOLD], localize.get(BASIC_TRIGGERS));

                            ImGui::TextUnformatted(localize_format(FORMAT_VISIBLE, visibleLabel).c_str());
                            ImGui::TextUnformatted(localize_format(FORMAT_TRIGGERS_COUNT, framesCount).c_str());
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

            // An animation layer's icon is a folder that shows (or hides) its child's tracks.
            auto isExpanded = item && expandedLayerTracks.contains(item->uid);
            if (animationLayer)
                icon = isExpanded ? ICON_FOLDER_OPEN : ICON_FOLDER;
            ImGui::Image(resources.icon_id_get(icon), icon_size_get());
            auto iconMin = ImGui::GetItemRectMin();
            auto iconMax = ImGui::GetItemRectMax();
            auto isIconHovered = is_mouse_in(iconMin, iconMax);
            if (animationLayer && isItemButtonHovered && isIconHovered &&
                ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left))
            {
                if (isExpanded)
                    expandedLayerTracks.erase(item->uid);
                else
                    expandedLayerTracks.insert(item->uid);
            }
            overlay_icon(resources.icon_id_get(icon), iconTintCurrent);
            ImGui::SameLine();
            if (isReferenced)
                ImGui::PushFont(resources.fonts[FONT_ITALICS], FONT_SIZE);
            ImGui::PushStyleColor(ImGuiCol_Text, itemTextColor);
            ImGui::TextUnformatted(label.c_str());
            ImGui::PopStyleColor();
            if (isReferenced)
                ImGui::PopFont();

            if (row_toggle_draw(itemSize, 1,
                                {"##Visible Toggle", isItemVisible, ICON_VISIBLE, ICON_INVISIBLE,
                                 TOOLTIP_ITEM_VISIBILITY_SHOWN, TOOLTIP_ITEM_VISIBILITY_HIDDEN},
                                iconTintCurrent))
            {
                auto animationIndex = reference.animationIndex;
                auto targetType = type;
                auto targetID = id;
                auto targetGroupType = row.groupType;
                auto targetGroupId = row.groupId;
                edit_push(EDIT_TOGGLE_ITEM_VISIBILITY, [=](Model& model) {
                    if (auto item = model.track_edit(
                            Reference{animationIndex, targetType, targetID, -1, targetGroupType, targetGroupId}))
                        item->isVisible = !item->isVisible;
                });
            }

            if (type == NULL_)
            {
                if (auto null = ::item_get(model.content.nulls, id))
                {
                    if (row_toggle_draw(itemSize, 2,
                                        {"##Rect Toggle", null->isShowRect, ICON_SHOW_RECT, ICON_HIDE_RECT,
                                         TOOLTIP_NULL_RECT_SHOWN, TOOLTIP_NULL_RECT_HIDDEN},
                                        iconTintCurrent))
                    {
                        auto nullID = id;
                        edit_push(EDIT_TOGGLE_NULL_RECT, [=](Model& model) {
                            if (auto null = ::item_get(model.content.nulls, nullID))
                                null->isShowRect = !null->isShowRect;
                        });
                    }
                }
            }
        }
        else
        {
            auto cursorPos = ImGui::GetCursorPos();
            auto& isShowUnused = settings.timelineIsShowUnused;
            if (row_toggle_draw(itemSize, 1,
                                {"##Unused Toggle", isShowUnused, ICON_SHOW_UNUSED, ICON_HIDE_UNUSED,
                                 TOOLTIP_UNUSED_ITEMS_SHOWN, TOOLTIP_UNUSED_ITEMS_HIDDEN},
                                iconTintCurrent))
                isShowUnused = !isShowUnused;
            auto& isOnlyLayers = settings.timelineIsOnlyShowLayers;
            if (row_toggle_draw(itemSize, 2,
                                {"##Layers Toggle", isOnlyLayers, ICON_SHOW_LAYERS, ICON_HIDE_LAYERS,
                                 TOOLTIP_ONLY_LAYERS_VISIBLE, TOOLTIP_ALL_ITEMS_VISIBLE},
                                iconTintCurrent))
                isOnlyLayers = !isOnlyLayers;

            ImGui::SetCursorPos(cursorPos);

            ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::Text("(?)");
            ImGui::PopStyleColor();
            auto tooltipShortcuts = std::vformat(
                localize.get(TOOLTIP_TIMELINE_SHORTCUTS),
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
                auto rowReferences = timeline_item_rows_get();
                row_selection_clear();
                for (auto rowReference : rowReferences)
                    row_selection_insert(rowReference);
                if (!rowReferences.empty())
                    rowSelectionAnchor = rowReferences.front();
                document.frame_references_clear();
            }

            if (animation && shortcut(manager.chords[SHORTCUT_GROUP], SCOPE_FOCUSED) &&
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

                auto item_child_row = [&](const TimelineRow& row, int index) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    item_child(row, index);
                };

                int index = 0;
                item_child_row({.type = NONE}, index++);
                if (animation)
                    for (auto& row : timeline_item_rows_get())
                    {
                        item_child_row(row, index++);
                        for (auto& peek : peek_rows_get(row))
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            peek_item_child(row, peek, index++);
                        }
                    }

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
            if (ImGui::Button(localize.get(BASIC_ADD), widgetSize))
                itemProperties.open();
            set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_ITEM), settings.shortcutAdd);
            ImGui::SameLine();

            auto selectedRows = selected_row_references_get();
            auto isRemoveAvailable = std::ranges::any_of(
                selectedRows, [](const TimelineRow& row) { return row.type == LAYER || row.type == NULL_; });
            ImGui::BeginDisabled(!isRemoveAvailable);
            shortcut(manager.chords[SHORTCUT_REMOVE]);
            if (ImGui::Button(localize.get(BASIC_REMOVE), widgetSize))
                item_remove();
            set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_ITEMS), settings.shortcutRemove);
            ImGui::EndDisabled();
        }
        ImGui::EndDisabled();

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
}

// Lets a row be dragged, and dragged rows land before or after it, or into it when it is a group of their kind.
void TimelineContext::row_drag_drop(const TimelineRow& row, ImVec2 buttonMin, ImVec2 buttonMax, ImVec2 childMin,
                                    ImVec2 childMax)
{
    if (ImGui::BeginDragDropSource(DRAG_DROP_SOURCE_FLAGS))
    {
        rowDragReferences = row_drag_references_get(row);
        ImGui::SetDragDropPayload(ROW_DRAG_DROP, rowDragReferences.data(),
                                  (int)rowDragReferences.size() * (int)sizeof(TimelineRow));
        ImGui::EndDragDropSource();
    }
    if (!ImGui::BeginDragDropTarget())
        return;
    if (auto payload = ImGui::AcceptDragDropPayload(ROW_DRAG_DROP, ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                                       ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
        auto rows = (TimelineRow*)payload->Data;
        std::vector<TimelineRow> draggedRows(rows, rows + payload->DataSize / sizeof(TimelineRow));
        auto zone = row.isGroup ? drop_zone_get(buttonMin, buttonMax)
                                : (is_drop_after(buttonMin, buttonMax) ? DROP_AFTER : DROP_BEFORE);
        auto isDropAfter = zone != DROP_BEFORE;
        auto isDropIntoGroup = zone == DROP_INSIDE && (row.type == LAYER || row.type == NULL_) &&
                               std::ranges::none_of(draggedRows, [&](const TimelineRow& dragged) {
                                   return dragged.isGroup || dragged.type != row.type;
                               });
        if (isDropIntoGroup)
            drop_box_draw(ImGui::GetWindowDrawList(), childMin, childMax);
        else
            drop_line_draw(ImGui::GetWindowDrawList(), childMin, childMax, isDropAfter);
        if (payload->IsDelivery())
            rows_move_to_row(draggedRows, row, isDropIntoGroup ? DROP_INSIDE : isDropAfter ? DROP_AFTER : DROP_BEFORE);
    }
    ImGui::EndDragDropTarget();
}

// An icon button at a row's right edge (slot 1 rightmost), drawn flat, showing the icon and tooltip for whether it is
// on. True when clicked. Its rectangle stays the last item's.
bool TimelineContext::row_toggle_draw(ImVec2 itemSize, int slot, const RowToggle& toggle, ImVec4 tint)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    auto lineHeight = ImGui::GetTextLineHeightWithSpacing();
    ImGui::SetCursorPos(
        ImVec2(itemSize.x - lineHeight * slot - ImGui::GetStyle().ItemSpacing.x, (itemSize.y - lineHeight) / 2));
    auto texture = resources.icon_id_get(toggle.isOn ? toggle.on : toggle.off);
    auto isClicked = ImGui::ImageButton(toggle.id, texture, icon_size_get());
    overlay_icon(texture, tint);
    tooltip_set(toggle.isOn ? toggle.onTooltip : toggle.offTooltip);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    return isClicked;
}
