#include "nulls.h"

#include <ranges>

using namespace anm2ed::document;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;

namespace anm2ed::nulls
{
  void Nulls::update(Document& document, int& documentIndex, Settings& settings, Resources& resources)
  {
    auto& anm2 = document.anm2;
    auto& selection = document.selectedNulls;

    if (document.is_just_changed(change::NULLS)) unusedNullsIDs = anm2.nulls_unused();

    storage.user_data_set(&selection);

    if (ImGui::Begin("Nulls", &settings.windowIsNulls))
    {
      auto childSize = imgui::size_without_footer_get();

      if (ImGui::BeginChild("##Nulls Child", childSize, true))
      {
        storage.begin(anm2.content.nulls.size());

        for (auto& [id, null] : anm2.content.nulls)
        {
          auto isSelected = selection.contains(id);

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          imgui::selectable_input_text(std::format("#{} {}", id, null.name),
                                       std::format("###Document #{} Null #{}", documentIndex, id), null.name,
                                       isSelected);
          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(null.name.c_str());
            ImGui::PopFont();
            ImGui::Text("ID: %d", id);
            ImGui::EndTooltip();
          }
          ImGui::PopID();
        }

        storage.end();
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      imgui::shortcut(settings.shortcutAdd);
      ImGui::Button("Add", widgetSize);
      imgui::set_item_tooltip_shortcut("Add a null.", settings.shortcutAdd);
      ImGui::SameLine();

      imgui::shortcut(settings.shortcutRemove);
      ImGui::BeginDisabled(unusedNullsIDs.empty());
      {
        if (ImGui::Button("Remove Unused", widgetSize))
          for (auto& id : unusedNullsIDs)
            anm2.content.nulls.erase(id);
        document.change(change::NULLS);
      }
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused nulls (i.e., ones not used in any animation.)",
                                       settings.shortcutRemove);
    }
    ImGui::End();
  }
}