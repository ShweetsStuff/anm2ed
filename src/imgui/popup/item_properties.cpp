#include "item_properties.hpp"

#include <format>

#include "imgui_.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui::popup
{
  void ItemProperties::reset()
  {
    addItemName.clear();
    addItemID = -1;
    addItemSpritesheetID = {};
  }

  std::set<int> ItemProperties::unused_items_get(anm2::Anm2& anm2, anm2::Animation* animation, anm2::Type type)
  {
    if (!animation) return {};
    if (type == anm2::LAYER) return anm2.layers_unused(*animation);
    if (type == anm2::NULL_) return anm2.nulls_unused(*animation);
    return {};
  }

  void ItemProperties::open()
  {
    reset();
    popup.open();
  }

  void ItemProperties::update(Manager& manager, Settings& settings, Document& document, anm2::Animation* animation,
                              anm2::Reference& reference,
                              const std::function<void(anm2::Type, int)>& referenceSetItem)
  {
    auto& anm2 = document.anm2;

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

      auto contentHeight = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing();
      if (contentHeight < 1.0f) contentHeight = 1.0f;

      if (ImGui::BeginChild("##Content", ImVec2(0, contentHeight), ImGuiChildFlags_Borders))
      {
        auto spaced_pair = [](const std::function<void()>& left, const std::function<void()>& right)
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
              ImGui::RadioButton(localize.get(LABEL_LAYER), &type, anm2::LAYER);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_TYPE));
            },
            [&]()
            {
              ImGui::RadioButton(localize.get(LABEL_NULL), &type, anm2::NULL_);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_TYPE));
            });

        ImGui::SeparatorText(localize.get(LABEL_SOURCE));

        auto isUnusedItems = animation && !unused_items_get(anm2, animation, (anm2::Type)type).empty();
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

          input_text_string(localize.get(BASIC_NAME), &addItemName);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
          if (type == anm2::LAYER)
          {
            combo_id_mapped(localize.get(LABEL_SPRITESHEET), &addItemSpritesheetID, document.spritesheet.ids,
                            document.spritesheet.labels);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_SPRITESHEET));
          }
          else if (type == anm2::NULL_)
          {
            ImGui::Checkbox(localize.get(LABEL_RECT), &addItemIsShowRect);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_RECT));
          }
        }
        else if (animation)
        {
          ImGui::SeparatorText(localize.get(type == anm2::LAYER ? LABEL_LAYER : LABEL_NULL));

          if (ImGui::BeginChild("##Existing Items", ImVec2(0, 0)))
          {
            auto unusedItems = unused_items_get(anm2, animation, (anm2::Type)type);
            if (addItemID != -1 && !unusedItems.contains(addItemID)) addItemID = -1;

            for (auto id : unusedItems)
            {
              auto isSelected = addItemID == id;

              ImGui::PushID(id);

              if (type == anm2::LAYER)
              {
                if (auto it = anm2.content.layers.find(id); it != anm2.content.layers.end())
                {
                  auto& layer = it->second;
                  auto label = std::vformat(localize.get(FORMAT_LAYER),
                                            std::make_format_args(id, layer.name, layer.spritesheetID));
                  if (ImGui::Selectable(label.c_str(), isSelected))
                  {
                    addItemID = id;
                    addItemSpritesheetID = layer.spritesheetID;
                  }
                }
              }
              else if (type == anm2::NULL_)
              {
                if (auto it = anm2.content.nulls.find(id); it != anm2.content.nulls.end())
                {
                  auto& null = it->second;
                  auto label = std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, null.name));
                  if (ImGui::Selectable(label.c_str(), isSelected))
                  {
                    addItemID = id;
                    addItemIsShowRect = null.isShowRect;
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

      ImGui::BeginDisabled(source == source::EXISTING && addItemID == -1);
      shortcut(manager.chords[SHORTCUT_CONFIRM]);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize))
      {
        anm2::Reference addReference{};
        int insertBeforeID = reference.itemType == anm2::LAYER ? reference.itemID : -1;

        document.snapshot(localize.get(EDIT_ADD_ITEM));
        if (type == anm2::LAYER)
          addReference = anm2.layer_animation_add({reference.animationIndex, anm2::LAYER, addItemID}, insertBeforeID,
                                                  addItemName, addItemSpritesheetID, (destination::Type)destination);
        else if (type == anm2::NULL_)
          addReference = anm2.null_animation_add({reference.animationIndex, anm2::NULL_, addItemID}, addItemName,
                                                 addItemIsShowRect, (destination::Type)destination);

        document.change(Document::ITEMS);

        referenceSetItem(addReference.itemType, addReference.itemID);

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
  }
}
