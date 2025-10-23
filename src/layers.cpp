#include "layers.h"

#include <ranges>

#include "util.h"

using namespace anm2ed::document;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::layers
{
  void Layers::update(Document& document, Settings& settings, Resources& resources)
  {
    auto& anm2 = document.anm2;
    auto& selection = document.selectedLayers;
    auto& referenceLayer = document.referenceLayer;

    if (document.is_just_changed(change::LAYERS)) unusedLayerIDs = anm2.layers_unused();

    storage.user_data_set(&selection);

    auto properties_popup_open = [&](int id = -1)
    {
      if (id == -1)
      {
        isAdd = true;
        editLayer = anm2::Layer();
      }
      else
        editLayer = anm2.content.layers.at(id);

      propertiesPopup.open();
    };

    if (ImGui::Begin("Layers", &settings.windowIsLayers))
    {
      auto childSize = imgui::size_without_footer_get();

      if (ImGui::BeginChild("##Layers Child", childSize, true))
      {
        storage.begin(anm2.content.layers.size());

        for (auto& [id, layer] : anm2.content.layers)
        {
          auto isSelected = selection.contains(id);
          auto isReferenced = referenceLayer == id;

          ImGui::PushID(id);

          ImGui::SetNextItemSelectionUserData(id);
          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          ImGui::Selectable(std::format("#{} {} (Spritesheet: #{})", id, layer.name, layer.spritesheetID).c_str(),
                            isSelected);
          if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
          {
            referenceLayer = id;
            properties_popup_open(id);
          }

          if (isReferenced) ImGui::PopFont();
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

        storage.end();
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      imgui::shortcut(settings.shortcutAdd);
      if (ImGui::Button("Add", widgetSize)) properties_popup_open();
      imgui::set_item_tooltip_shortcut("Add a layer.", settings.shortcutAdd);
      ImGui::SameLine();

      imgui::shortcut(settings.shortcutRemove);
      ImGui::BeginDisabled(unusedLayerIDs.empty());
      {
        if (ImGui::Button("Remove Unused", widgetSize))
        {
          for (auto& id : unusedLayerIDs)
            anm2.content.layers.erase(id);
          document.change(change::LAYERS);
          unusedLayerIDs.clear();
        }
      }
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused layers (i.e., ones not used in any animation.)",
                                       settings.shortcutRemove);
    }
    ImGui::End();

    propertiesPopup.trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label, &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = imgui::child_size_get(2);
      auto& layer = editLayer;

      auto close = [&]()
      {
        isAdd = false;
        editLayer = anm2::Layer();
        propertiesPopup.close();
      };

      if (ImGui::BeginChild("Child", childSize, ImGuiChildFlags_Borders))
      {
        imgui::input_text_string("Name", &layer.name);
        ImGui::SetItemTooltip("Set the item's name.");
        imgui::combo_strings("Spritesheet", &layer.spritesheetID, document.spritesheetNames);
        ImGui::SetItemTooltip("Set the layer item's spritesheet.");
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      if (ImGui::Button(isAdd ? "Add" : "Confirm", widgetSize))
      {
        if (isAdd)
        {
          auto id = map::next_id_get(anm2.content.layers);
          anm2.content.layers[id] = editLayer;
          referenceLayer = id;
        }
        else
          anm2.content.layers[referenceLayer] = editLayer;
        document.change(change::LAYERS);
        close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) close();

      ImGui::EndPopup();
    }

    referenceLayer = propertiesPopup.isOpen ? referenceLayer : -1;
  }
}
