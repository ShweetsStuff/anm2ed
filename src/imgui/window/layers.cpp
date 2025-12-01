#include "layers.h"

#include <ranges>

#include "log.h"
#include "map_.h"
#include "strings.h"
#include "toast.h"

using namespace anm2ed::util;
using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui
{
  void Layers::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.layer.reference;
    auto& unused = document.layer.unused;
    auto& hovered = document.layer.hovered;
    auto& selection = document.layer.selection;
    auto& propertiesPopup = manager.layerPropertiesPopup;

    hovered = -1;

    if (ImGui::Begin(localize.get(LABEL_LAYERS_WINDOW), &settings.windowIsLayers))
    {
      auto childSize = size_without_footer_get();

      if (ImGui::BeginChild("##Layers Child", childSize, true))
      {
        selection.start(anm2.content.layers.size());

        for (auto& [id, layer] : anm2.content.layers)
        {
          auto isSelected = selection.contains(id);

          ImGui::PushID(id);

          ImGui::SetNextItemSelectionUserData(id);
          ImGui::Selectable(
              std::vformat(localize.get(FORMAT_LAYER), std::make_format_args(id, layer.name, layer.spritesheetID))
                  .c_str(),
              isSelected);
          if (newLayerId == id)
          {
            ImGui::SetScrollHereY(0.5f);
            newLayerId = -1;
          }
          if (ImGui::IsItemHovered())
          {
            hovered = id;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) manager.layer_properties_open(id);
          }
          else
            hovered = -1;

          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(layer.name.c_str());
            ImGui::PopFont();
            ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_SPRITESHEET_ID), std::make_format_args(layer.spritesheetID)).c_str());
            ImGui::EndTooltip();
          }
          ImGui::PopID();
        }

        selection.finish();

        auto copy = [&]()
        {
          if (!selection.empty())
          {
            std::string clipboardText{};
            for (auto& id : selection)
              clipboardText += anm2.content.layers[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.layers[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          std::string errorString{};
          document.snapshot(localize.get(EDIT_PASTE_LAYERS));
          if (anm2.layers_deserialize(clipboard.get(), type, &errorString))
            document.change(Document::NULLS);
          else
          {
            toasts.push(std::vformat(localize.get(TOAST_DESERIALIZE_LAYERS_FAILED),
                                     std::make_format_args(errorString)));
            logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_LAYERS_FAILED, anm2ed::ENGLISH),
                                      std::make_format_args(errorString)));
          }
        };

        if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste(merge::APPEND);

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::MenuItem(localize.get(BASIC_CUT), settings.shortcutCut.c_str(), false, false);
          if (ImGui::MenuItem(localize.get(BASIC_COPY), settings.shortcutCopy.c_str(), false,
                              !selection.empty() || hovered > -1))
            copy();

          if (ImGui::BeginMenu(localize.get(BASIC_PASTE), !clipboard.is_empty()))
          {
            if (ImGui::MenuItem(localize.get(BASIC_APPEND), settings.shortcutPaste.c_str())) paste(merge::APPEND);
            if (ImGui::MenuItem(localize.get(BASIC_REPLACE))) paste(merge::REPLACE);

            ImGui::EndMenu();
          }

          ImGui::EndPopup();
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize)) manager.layer_properties_open();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_LAYER), settings.shortcutAdd);
      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_REMOVE]);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), widgetSize))
      {
        auto remove_unused = [&]()
        {
          for (auto& id : unused)
            anm2.content.layers.erase(id);
          unused.clear();
        };

        DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_LAYERS), Document::LAYERS, remove_unused());
      }
      ImGui::EndDisabled();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_LAYERS), settings.shortcutRemove);
    }
    ImGui::End();

    manager.layer_properties_trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label(), &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = child_size_get(2);
      auto& layer = manager.editLayer;

      if (ImGui::BeginChild("##Child", childSize, ImGuiChildFlags_Borders))
      {
        if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string(localize.get(BASIC_NAME), &layer.name);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
        combo_negative_one_indexed(localize.get(LABEL_SPRITESHEET), &layer.spritesheetID, document.spritesheet.labels);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_SPRITESHEET));
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(reference == -1 ? localize.get(BASIC_ADD) : localize.get(BASIC_CONFIRM), widgetSize))
      {
        if (reference == -1)
        {
          auto add = [&]()
          {
            auto id = map::next_id_get(anm2.content.layers);
            anm2.content.layers[id] = layer;
            selection = {id};
            newLayerId = id;
          };

          DOCUMENT_EDIT(document, localize.get(EDIT_ADD_LAYER), Document::LAYERS, add());
        }
        else
        {
          auto set = [&]()
          {
            anm2.content.layers[reference] = layer;
            selection = {reference};
          };

          DOCUMENT_EDIT(document, localize.get(EDIT_SET_LAYER_PROPERTIES), Document::LAYERS, set());
        }

        manager.layer_properties_close();
      }

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) manager.layer_properties_close();

      manager.layer_properties_end();
      ImGui::EndPopup();
    }
  }
}
