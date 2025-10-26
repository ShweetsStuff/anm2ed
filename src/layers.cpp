#include "layers.h"

#include <ranges>

using namespace anm2ed::document;
using namespace anm2ed::clipboard;
using namespace anm2ed::manager;
using namespace anm2ed::resources;
using namespace anm2ed::settings;
using namespace anm2ed::types;

namespace anm2ed::layers
{
  void Layers::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.referenceLayer;
    auto& unused = document.unusedLayerIDs;
    auto& hovered = document.hoveredLayer;
    auto& multiSelect = document.layersMultiSelect;
    auto& propertiesPopup = manager.layerPropertiesPopup;

    if (ImGui::Begin("Layers", &settings.windowIsLayers))
    {
      auto childSize = imgui::size_without_footer_get();

      if (ImGui::BeginChild("##Layers Child", childSize, true))
      {
        multiSelect.start(anm2.content.layers.size());

        for (auto& [id, layer] : anm2.content.layers)
        {
          auto isSelected = multiSelect.contains(id);

          ImGui::PushID(id);

          ImGui::SetNextItemSelectionUserData(id);
          ImGui::Selectable(std::format(anm2::LAYER_FORMAT, id, layer.name, layer.spritesheetID).c_str(), isSelected);
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
            ImGui::Text("ID: %d", id);
            ImGui::Text("Spritesheet ID: %d", layer.spritesheetID);
            ImGui::EndTooltip();
          }
          ImGui::PopID();
        }

        multiSelect.finish();

        auto copy = [&]()
        {
          if (!multiSelect.empty())
          {
            std::string clipboardText{};
            for (auto& id : multiSelect)
              clipboardText += anm2.content.layers[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.layers[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          auto clipboardText = clipboard.get();
          document.layers_deserialize(clipboardText, type);
        };

        if (imgui::shortcut(settings.shortcutCopy, shortcut::FOCUSED)) copy();
        if (imgui::shortcut(settings.shortcutPaste, shortcut::FOCUSED)) paste(merge::APPEND);

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem("Cut", settings.shortcutCut.c_str());
          ImGui::EndDisabled();

          ImGui::BeginDisabled(multiSelect.empty() && hovered == -1);
          if (ImGui::MenuItem("Copy", settings.shortcutCopy.c_str())) copy();
          ImGui::EndDisabled();

          ImGui::BeginDisabled(clipboard.is_empty());
          {
            if (ImGui::BeginMenu("Paste"))
            {
              if (ImGui::MenuItem("Append", settings.shortcutPaste.c_str())) paste(merge::APPEND);
              if (ImGui::MenuItem("Replace")) paste(merge::REPLACE);

              ImGui::EndMenu();
            }
          }
          ImGui::EndDisabled();

          ImGui::EndPopup();
        }
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      imgui::shortcut(settings.shortcutAdd);
      if (ImGui::Button("Add", widgetSize)) manager.layer_properties_open();
      imgui::set_item_tooltip_shortcut("Add a layer.", settings.shortcutAdd);
      ImGui::SameLine();

      imgui::shortcut(settings.shortcutRemove);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button("Remove Unused", widgetSize)) document.layers_remove_unused();
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused layers (i.e., ones not used in any animation.)",
                                       settings.shortcutRemove);
    }
    ImGui::End();

    manager.layer_properties_trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label, &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = imgui::child_size_get(2);
      auto& layer = manager.editLayer;

      if (ImGui::BeginChild("Child", childSize, ImGuiChildFlags_Borders))
      {
        if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
        imgui::input_text_string("Name", &layer.name);
        ImGui::SetItemTooltip("Set the item's name.");
        imgui::combo_strings("Spritesheet", &layer.spritesheetID, document.spritesheetNames);
        ImGui::SetItemTooltip("Set the layer item's spritesheet.");
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      if (ImGui::Button(reference == -1 ? "Add" : "Confirm", widgetSize))
      {
        document.layer_set(layer);
        manager.layer_properties_close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) manager.layer_properties_close();

      manager.layer_properties_end();
      ImGui::EndPopup();
    }
  }
}
