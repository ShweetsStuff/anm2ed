#include "item_properties.hpp"

#include <format>

#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"

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

  std::set<int> ItemProperties::unused_items_get(Anm2& anm2, const Element* animation, int type)
  {
    if (!animation) return {};
    if (type == LAYER) return anm2.element_unused(ElementType::LAYER_ELEMENT, *animation);
    if (type == NULL_) return anm2.element_unused(ElementType::NULL_ELEMENT, *animation);
    return {};
  }

  void ItemProperties::open()
  {
    reset();
    popup.open();
  }

  bool ItemProperties::update(Manager& manager, Settings& settings, Document& document, Reference& reference)
  {
    bool isAdded{};
    auto& anm2 = document.anm2;
    auto animation = anm2.element_get(ElementType::ANIMATION, reference.animationIndex);

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

        auto isUnusedItems = animation && !unused_items_get(anm2, animation, (int)type).empty();
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
          if (type == LAYER)
          {
            combo_id_mapped(localize.get(LABEL_SPRITESHEET), &addItemSpritesheetID, document.spritesheet.ids,
                            document.spritesheet.labels);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_SPRITESHEET));
          }
          else if (type == NULL_)
          {
            ImGui::Checkbox(localize.get(LABEL_RECT), &addItemIsShowRect);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_RECT));
          }
        }
        else if (animation)
        {
          ImGui::SeparatorText(localize.get(type == LAYER ? LABEL_LAYER : LABEL_NULL));

          if (ImGui::BeginChild("##Existing Items", ImVec2(0, 0)))
          {
            auto unusedItems = unused_items_get(anm2, animation, (int)type);
            if (addItemID != -1 && !unusedItems.contains(addItemID)) addItemID = -1;

            for (auto id : unusedItems)
            {
              auto isSelected = addItemID == id;

              ImGui::PushID(id);

              if (type == LAYER)
              {
                if (auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, id))
                {
                  auto label = std::vformat(localize.get(FORMAT_LAYER),
                                            std::make_format_args(id, layer->name, layer->spritesheetId));
                  if (ImGui::Selectable(label.c_str(), isSelected))
                  {
                    addItemID = id;
                    addItemSpritesheetID = layer->spritesheetId;
                  }
                }
              }
              else if (type == NULL_)
              {
                auto nulls = anm2.element_get(ElementType::NULLS);
                auto null = nulls ? element_child_id_get(*nulls, ElementType::NULL_ELEMENT, id) : nullptr;
                if (null)
                {
                  auto label = std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, null->name));
                  if (ImGui::Selectable(label.c_str(), isSelected))
                  {
                    addItemID = id;
                    addItemIsShowRect = null->isShowRect;
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
        auto queuedType = type;
        auto queuedDestination = destination;
        auto queuedAnimationIndex = reference.animationIndex;
        auto queuedInsertBeforeID = reference.itemType == LAYER ? reference.itemID : -1;
        auto queuedAddItemID = addItemID;
        auto queuedAddItemName = addItemName;
        auto queuedAddItemSpritesheetID = addItemSpritesheetID;
        auto queuedAddItemIsShowRect = addItemIsShowRect;

        manager.command_push({manager.selected,
                              [=](Manager&, Document& document)
                              {
                                int addId{-1};

                                document.snapshot(localize.get(EDIT_ADD_ITEM));
                                if (queuedType == LAYER)
                                  addId = document.anm2.layer_animation_add(
                                      queuedAnimationIndex, queuedAddItemID, queuedInsertBeforeID, queuedAddItemName,
                                      queuedAddItemSpritesheetID, (destination::Type)queuedDestination);
                                else if (queuedType == NULL_)
                                  addId = document.anm2.null_animation_add(queuedAnimationIndex, queuedAddItemID,
                                                                           queuedAddItemName, queuedAddItemIsShowRect,
                                                                           (destination::Type)queuedDestination);

                                document.anm2_change(Document::ITEMS);

                                if (addId != -1)
                                {
                                  document.reference = {queuedAnimationIndex, (int)queuedType, addId};
                                  document.items.references = {document.reference};
                                  document.frames.selection.clear();
                                  document.frames.references.clear();
                                  if (queuedType == LAYER)
                                    if (auto layer = document.anm2.element_get(ElementType::LAYER_ELEMENT, addId))
                                      document.spritesheet.reference = layer->spritesheetId;
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
