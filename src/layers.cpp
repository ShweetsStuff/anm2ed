#include "layers.h"

#include <ranges>

#include "imgui.h"

using namespace anm2ed::document_manager;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;

namespace anm2ed::layers
{
  void Layers::update(DocumentManager& manager, Settings& settings, Resources& resources)
  {
    if (ImGui::Begin("Layers", &settings.windowIsLayers))
    {
      auto document = manager.get();
      anm2::Anm2& anm2 = document->anm2;

      auto& selection = document->selectedLayers;
      storage.UserData = &selection;
      storage.AdapterSetItemSelected = imgui::external_storage_set;

      auto childSize = imgui::size_with_footer_get();

      if (ImGui::BeginChild("##Layers Child", childSize, true))
      {
        ImGuiMultiSelectIO* io =
            ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_ClearOnEscape, selection.size(), anm2.content.layers.size());
        storage.ApplyRequests(io);

        for (auto& [id, layer] : anm2.content.layers)
        {
          auto isSelected = selection.contains(id);

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          imgui::selectable_input_text(std::format("#{} {}", id, layer.name),
                                       std::format("###Document #{} Layer #{}", manager.selected, id), layer.name,
                                       isSelected);
          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(layer.name.c_str());
            ImGui::TextUnformatted(std::format("ID: {}", id).c_str());
            ImGui::TextUnformatted(std::format("Spritesheet ID: {}", layer.spritesheetID).c_str());
            ImGui::PopFont();
            ImGui::EndTooltip();
          }
          ImGui::PopID();
        }

        io = ImGui::EndMultiSelect();
        storage.ApplyRequests(io);
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      imgui::shortcut(settings.shortcutAdd, true);
      ImGui::Button("Add", widgetSize);
      imgui::set_item_tooltip_shortcut("Add a layer.", settings.shortcutAdd);
      ImGui::SameLine();

      std::set<int> unusedLayersIDs = anm2.layers_unused();

      imgui::shortcut(settings.shortcutRemove, true);
      ImGui::BeginDisabled(unusedLayersIDs.empty());
      {
        if (ImGui::Button("Remove Unused", widgetSize))
          for (auto& id : unusedLayersIDs)
            anm2.content.layers.erase(id);
      }
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused layers (i.e., ones not used in any animation.)",
                                       settings.shortcutRemove);
    }
    ImGui::End();
  }
}
