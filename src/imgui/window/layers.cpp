#include "layers.h"

#include <ranges>

#include "map_.h"
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

    if (ImGui::Begin("Layers", &settings.windowIsLayers))
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
          document.snapshot("Paste Layer(s)");
          if (anm2.layers_deserialize(clipboard.get(), type, &errorString))
            document.change(Document::NULLS);
          else
            toasts.error(std::format("Failed to deserialize layer(s): {}", errorString));
        };

        if (shortcut(settings.shortcutCopy, shortcut::FOCUSED)) copy();
        if (shortcut(settings.shortcutPaste, shortcut::FOCUSED)) paste(merge::APPEND);

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::MenuItem("Cut", settings.shortcutCut.c_str(), false, false);
          if (ImGui::MenuItem("Copy", settings.shortcutCopy.c_str(), false, !selection.empty() || hovered > -1)) copy();

          if (ImGui::BeginMenu("Paste", !clipboard.is_empty()))
          {
            if (ImGui::MenuItem("Append", settings.shortcutPaste.c_str())) paste(merge::APPEND);
            if (ImGui::MenuItem("Replace")) paste(merge::REPLACE);

            ImGui::EndMenu();
          }

          ImGui::EndPopup();
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(settings.shortcutAdd);
      if (ImGui::Button("Add", widgetSize)) manager.layer_properties_open();
      set_item_tooltip_shortcut("Add a layer.", settings.shortcutAdd);
      ImGui::SameLine();

      shortcut(settings.shortcutRemove);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button("Remove Unused", widgetSize))
      {
        auto remove_unused = [&]()
        {
          for (auto& id : unused)
            anm2.content.layers.erase(id);
          unused.clear();
        };

        DOCUMENT_EDIT(document, "Remove Unused Layers", Document::LAYERS, remove_unused());
      }
      ImGui::EndDisabled();
      set_item_tooltip_shortcut("Remove unused layers (i.e., ones not used in any animation.)",
                                settings.shortcutRemove);
    }
    ImGui::End();

    manager.layer_properties_trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label, &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = child_size_get(2);
      auto& layer = manager.editLayer;

      if (ImGui::BeginChild("Child", childSize, ImGuiChildFlags_Borders))
      {
        if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string("Name", &layer.name);
        ImGui::SetItemTooltip("Set the item's name.");
        combo_negative_one_indexed("Spritesheet", &layer.spritesheetID, document.spritesheet.labels);
        ImGui::SetItemTooltip("Set the layer item's spritesheet.");
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(reference == -1 ? "Add" : "Confirm", widgetSize))
      {
        auto add = [&]()
        {
          auto id = map::next_id_get(anm2.content.layers);
          anm2.content.layers[id] = layer;
          selection = {id};
        };

        auto set = [&]()
        {
          anm2.content.layers[reference] = layer;
          selection = {reference};
        };

        if (reference == -1)
        {
          DOCUMENT_EDIT(document, "Add Layer", Document::LAYERS, add());
        }
        else
          DOCUMENT_EDIT(document, "Set Layer Properties", Document::LAYERS, set());

        manager.layer_properties_close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) manager.layer_properties_close();

      manager.layer_properties_end();
      ImGui::EndPopup();
    }
  }
}
