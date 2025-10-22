#include "nulls.h"

#include <ranges>

#include "imgui.h"

using namespace anm2ed::document_manager;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;

namespace anm2ed::nulls
{
  void Nulls::update(DocumentManager& manager, Settings& settings, Resources& resources)
  {
    if (ImGui::Begin("Nulls", &settings.windowIsNulls))
    {
      auto document = manager.get();
      anm2::Anm2& anm2 = document->anm2;

      auto& selection = document->selectedNulls;
      storage.UserData = &selection;
      storage.AdapterSetItemSelected = imgui::external_storage_set;

      auto childSize = imgui::size_with_footer_get();

      if (ImGui::BeginChild("##Nulls Child", childSize, true))
      {
        ImGuiMultiSelectIO* io =
            ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_ClearOnEscape, selection.size(), anm2.content.nulls.size());
        storage.ApplyRequests(io);

        for (auto& [id, null] : anm2.content.nulls)
        {
          const bool isSelected = selection.contains(id);

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          imgui::selectable_input_text(std::format("#{} {}", id, null.name),
                                       std::format("###Document #{} Null #{}", manager.selected, id), null.name,
                                       isSelected);
          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(null.name.c_str());
            ImGui::TextUnformatted(std::format("ID: {}", id).c_str());
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
      imgui::set_item_tooltip_shortcut("Add a null.", settings.shortcutAdd);
      ImGui::SameLine();

      std::set<int> unusedNullsIDs = anm2.nulls_unused();

      imgui::shortcut(settings.shortcutRemove, true);
      ImGui::BeginDisabled(unusedNullsIDs.empty());
      {
        if (ImGui::Button("Remove Unused", widgetSize))
          for (auto& id : unusedNullsIDs)
            anm2.content.nulls.erase(id);
      }
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused nulls (i.e., ones not used in any animation.)",
                                       settings.shortcutRemove);
    }
    ImGui::End();
  }
}