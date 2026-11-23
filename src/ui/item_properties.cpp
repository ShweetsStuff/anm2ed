#include "ui/item_properties.h"

#include <format>

#include "ui/panel.h"
#include "ui/input.h"
#include "ui/layout.h"
#include "ui/shortcut.h"

constexpr int EXISTING_ITEMS_ROWS = 6;

void ItemProperties::reset()
{
    edit = {};
}

void ItemProperties::open()
{
    reset();
    popup.open();
}

bool ItemProperties::update(Manager& manager, Settings& settings, Document& document, Reference& reference)
{
    bool isAdded = false;
    auto& model = document.model;
    auto animation = model.animation_get(reference.animationIndex);
    auto unused_items_get = [&](int type) {
        return animation
                   ? model.unused_get(type == LAYER ? ElementType::LAYER_ELEMENT : ElementType::NULL_ELEMENT, animation)
                   : std::set<int>{};
    };

    if (popup.begin())
    {
        auto close = [&]() {
            reset();
            popup.close();
        };

        auto& type = settings.timelineAddItemType;
        auto& destination = settings.timelineAddItemDestination;
        auto& source = settings.timelineAddItemSource;

        if (ImGui::BeginChild("##Content", ImVec2(), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
        {
            auto spaced_pair = [](auto left, auto right) {
                auto startX = ImGui::GetCursorPosX();
                auto secondX = startX + ImGui::GetContentRegionAvail().x * 0.5f;
                left();
                ImGui::SameLine();
                ImGui::SetCursorPosX(secondX);
                right();
            };

            ImGui::SeparatorText(localize.get(LABEL_TYPE));

            spaced_pair([&]() { radio_draw(LABEL_LAYER, type, LAYER, TOOLTIP_LAYER_TYPE); },
                        [&]() { radio_draw(LABEL_NULL, type, NULL_, TOOLTIP_NULL_TYPE); });

            ImGui::SeparatorText(localize.get(LABEL_SOURCE));

            auto isUnusedItems = animation && !unused_items_get(type).empty();
            spaced_pair([&]() { radio_draw(LABEL_NEW, source, SOURCE_NEW, TOOLTIP_NEW_ITEM); },
                        [&]() {
                            ImGui::BeginDisabled(!isUnusedItems);
                            ImGui::RadioButton(localize.get(LABEL_EXISTING), &source, SOURCE_EXISTING);
                            ImGui::EndDisabled();
                            ImGui::SetItemTooltip("%s", isUnusedItems ? localize.get(TOOLTIP_USE_EXISTING_ITEM)
                                                                      : localize.get(TOOLTIP_NO_UNUSED_ITEMS));
                        });

            ImGui::SeparatorText(localize.get(LABEL_DESTINATION));

            spaced_pair(
                [&]() { radio_draw(LABEL_ALL_ANIMATIONS, destination, DESTINATION_ALL, TOOLTIP_ITEM_ALL_ANIMATIONS); },
                [&]() {
                    radio_draw(LABEL_THIS_ANIMATION, destination, DESTINATION_THIS, TOOLTIP_ITEM_THIS_ANIMATION);
                });

            if (source == SOURCE_NEW)
            {
                ImGui::SeparatorText(localize.get(LABEL_OPTIONS));

                input_text_string(localize.get(BASIC_NAME), &edit.name);
                tooltip_set(TOOLTIP_ITEM_NAME);
                if (type == LAYER)
                    layer_source_draw(document, edit);
                else if (type == NULL_)
                    checkbox_draw(LABEL_RECT, edit.isShowRect, TOOLTIP_NULL_RECT);
            }
            else if (animation)
            {
                ImGui::SeparatorText(localize.get(type == LAYER ? LABEL_LAYER : LABEL_NULL));

                if (ImGui::BeginChild("##Existing Items",
                                      ImVec2(0, ImGui::GetFrameHeightWithSpacing() * EXISTING_ITEMS_ROWS)))
                {
                    auto unusedItems = unused_items_get(type);
                    if (edit.id != -1 && !unusedItems.contains(edit.id))
                        edit.id = -1;

                    for (auto id : unusedItems)
                    {
                        auto isSelected = edit.id == id;

                        ImGui::PushID(id);

                        if (type == LAYER)
                        {
                            if (auto layer = item_get(model.content.layers, id))
                            {
                                if (ImGui::Selectable(layer_label_get(*layer).c_str(), isSelected))
                                    edit.id = id;
                            }
                        }
                        else if (type == NULL_)
                        {
                            if (auto null = item_get(model.content.nulls, id))
                            {
                                auto label = localize_format(FORMAT_NULL, id, null->name);
                                if (ImGui::Selectable(label.c_str(), isSelected))
                                    edit.id = id;
                            }
                        }

                        ImGui::PopID();
                    }
                }
                ImGui::EndChild();
            }
        }
        ImGui::EndChild();

        auto result = popup_buttons_draw(manager, {.confirm = BASIC_ADD,
                                                   .confirmTooltip = TOOLTIP_ADD_ITEM,
                                                   .cancelTooltip = TOOLTIP_CANCEL_ADD_ITEM,
                                                   .isConfirmEnabled = source != SOURCE_EXISTING || edit.id != -1});
        if (result == PopupButton::CONFIRM)
        {
            ItemAdd add = {.type = (ItemType)type,
                           .animationIndex = reference.animationIndex,
                           .id = source == SOURCE_NEW ? -1 : edit.id,
                           .insertBeforeId = reference.itemType == LAYER && type == LAYER ? reference.itemID : -1,
                           .destination = (Destination)destination,
                           .layer = item_edit_layer_get(edit),
                           .null = item_edit_null_get(edit)};

            manager.command_push(
                {manager.selected, [=](Manager&, Document& document) {
                     int addId = -1;
                     document.edit_apply(EDIT_ADD_ITEM, [&](Model& model) { addId = item_add(model, add); });

                     if (addId != -1)
                     {
                         document.reference_set({add.animationIndex, (int)add.type, addId});
                         document.selected_set(SelectionKind::TRACKS, {document.reference_get()});
                         document.frame_references_clear();
                         if (add.type == ItemType::LAYER)
                             if (auto layer = item_get(document.model.content.layers, addId))
                                 document.focused_id_set(SelectionKind::SPRITESHEETS, layer->spritesheetId);
                     }
                 }});

            isAdded = true;
        }
        if (result != PopupButton::NONE)
            close();

        ImGui::EndPopup();
    }

    return isAdded;
}
