#include "item_properties.hpp"

#include <format>

#include "../window/panel.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui::popup
{
  constexpr int EXISTING_ITEMS_ROWS = 6;

  void ItemProperties::reset() { edit = {}; }

  void ItemProperties::open()
  {
    reset();
    popup.open();
  }

  bool ItemProperties::update(Manager& manager, Settings& settings, Document& document, Reference& reference)
  {
    bool isAdded{};
    auto& model = document.model;
    auto animation = model.animation_get(reference.animationIndex);
    auto unused_items_get = [&](int type)
    {
      return animation
                 ? model.unused_get(type == LAYER ? ElementType::LAYER_ELEMENT : ElementType::NULL_ELEMENT, animation)
                 : std::set<int>{};
    };

    popup.trigger();

    if (ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto close = [&]()
      {
        reset();
        popup.close();
      };

      auto& type = settings.timelineAddItemType;
      auto& destination = settings.timelineAddItemDestination;
      auto& source = settings.timelineAddItemSource;

      if (ImGui::BeginChild("##Content", ImVec2(), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
      {
        auto spaced_pair = [](auto left, auto right)
        {
          auto startX = ImGui::GetCursorPosX();
          auto secondX = startX + ImGui::GetContentRegionAvail().x * 0.5f;
          left();
          ImGui::SameLine();
          ImGui::SetCursorPosX(secondX);
          right();
        };

        ImGui::SeparatorText(localize.get(LABEL_TYPE));

        spaced_pair(
            [&]()
            {
              ImGui::RadioButton(localize.get(LABEL_LAYER), &type, LAYER);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_TYPE));
            },
            [&]()
            {
              ImGui::RadioButton(localize.get(LABEL_NULL), &type, NULL_);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_TYPE));
            });

        ImGui::SeparatorText(localize.get(LABEL_SOURCE));

        auto isUnusedItems = animation && !unused_items_get(type).empty();
        spaced_pair(
            [&]()
            {
              ImGui::RadioButton(localize.get(LABEL_NEW), &source, source::NEW);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NEW_ITEM));
            },
            [&]()
            {
              ImGui::BeginDisabled(!isUnusedItems);
              ImGui::RadioButton(localize.get(LABEL_EXISTING), &source, source::EXISTING);
              ImGui::EndDisabled();
              ImGui::SetItemTooltip("%s", isUnusedItems ? localize.get(TOOLTIP_USE_EXISTING_ITEM)
                                                        : localize.get(TOOLTIP_NO_UNUSED_ITEMS));
            });

        ImGui::SeparatorText(localize.get(LABEL_DESTINATION));

        spaced_pair(
            [&]()
            {
              ImGui::RadioButton(localize.get(LABEL_ALL_ANIMATIONS), &destination, destination::ALL);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_ALL_ANIMATIONS));
            },
            [&]()
            {
              ImGui::RadioButton(localize.get(LABEL_THIS_ANIMATION), &destination, destination::THIS);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_THIS_ANIMATION));
            });

        if (source == source::NEW)
        {
          ImGui::SeparatorText(localize.get(LABEL_OPTIONS));

          input_text_string(localize.get(BASIC_NAME), &edit.name);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
          if (type == LAYER)
            layer_source_draw(document, edit);
          else if (type == NULL_)
          {
            ImGui::Checkbox(localize.get(LABEL_RECT), &edit.isShowRect);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_RECT));
          }
        }
        else if (animation)
        {
          ImGui::SeparatorText(localize.get(type == LAYER ? LABEL_LAYER : LABEL_NULL));

          if (ImGui::BeginChild("##Existing Items",
                                ImVec2(0, ImGui::GetFrameHeightWithSpacing() * EXISTING_ITEMS_ROWS)))
          {
            auto unusedItems = unused_items_get(type);
            if (edit.id != -1 && !unusedItems.contains(edit.id)) edit.id = -1;

            for (auto id : unusedItems)
            {
              auto isSelected = edit.id == id;

              ImGui::PushID(id);

              if (type == LAYER)
              {
                if (auto layer = model::item_get(model.content.layers, id))
                {
                  auto label = std::vformat(localize.get(FORMAT_LAYER),
                                            std::make_format_args(id, layer->name, layer->spritesheetId));
                  if (ImGui::Selectable(label.c_str(), isSelected))
                  {
                    edit.id = id;
                  }
                }
              }
              else if (type == NULL_)
              {
                if (auto null = model::item_get(model.content.nulls, id))
                {
                  auto label = std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, null->name));
                  if (ImGui::Selectable(label.c_str(), isSelected))
                  {
                    edit.id = id;
                  }
                }
              }

              ImGui::PopID();
            }
          }
          ImGui::EndChild();
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      ImGui::BeginDisabled(source == source::EXISTING && edit.id == -1);
      shortcut(manager.chords[SHORTCUT_CONFIRM]);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize))
      {
        auto queuedType = type;
        auto queuedDestination = destination;
        auto queuedAnimationIndex = reference.animationIndex;
        auto queuedInsertBeforeID = reference.itemType == LAYER && type == LAYER ? reference.itemID : -1;
        auto queuedAddItemID = source == source::NEW ? -1 : edit.id;
        auto queuedLayer = item_edit_layer_get(edit);
        auto queuedNull = item_edit_null_get(edit);

        manager.command_push({manager.selected, [=](Manager&, Document& document)
                              {
                                int addId{-1};
                                document.edit_apply(EDIT_ADD_ITEM,
                                                    [&](model::Model& model)
                                                    {
                                                      addId = edit::item_add(
                                                          model, (ItemType)queuedType, queuedAnimationIndex,
                                                          queuedAddItemID, queuedLayer, queuedNull,
                                                          queuedInsertBeforeID, (destination::Type)queuedDestination);
                                                    });

                                if (addId != -1)
                                {
                                  document.reference_set({queuedAnimationIndex, (int)queuedType, addId});
                                  document.selected_set(SelectionKind::TRACKS, {document.reference_get()});
                                  document.frame_references_clear();
                                  if (queuedType == LAYER)
                                    if (auto layer = model::item_get(document.model.content.layers, addId))
                                      document.focused_id_set(SelectionKind::SPRITESHEETS, layer->spritesheetId);
                                }
                              }});

        isAdded = true;
        close();
      }
      ImGui::EndDisabled();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADD_ITEM));

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) close();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CANCEL_ADD_ITEM));

      ImGui::EndPopup();
    }

    return isAdded;
  }
}
