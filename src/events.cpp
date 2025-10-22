#include "events.h"

#include <ranges>

#include "imgui.h"

using namespace anm2ed::document_manager;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;

namespace anm2ed::events
{
  void Events::update(DocumentManager& manager, Settings& settings, Resources& resources)
  {
    if (ImGui::Begin("Events", &settings.windowIsEvents))
    {
      auto document = manager.get();
      anm2::Anm2& anm2 = document->anm2;

      auto& selection = document->selectedEvents;
      storage.UserData = &selection;
      storage.AdapterSetItemSelected = imgui::external_storage_set;

      auto childSize = imgui::size_with_footer_get();

      if (ImGui::BeginChild("##Events Child", childSize, true))
      {
        ImGuiMultiSelectIO* io =
            ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_ClearOnEscape, selection.size(), anm2.content.events.size());
        storage.ApplyRequests(io);

        for (auto& [id, event] : anm2.content.events)
        {
          auto isSelected = selection.contains(id);

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          imgui::selectable_input_text(event.name, std::format("###Document #{} Event #{}", manager.selected, id),
                                       event.name, isSelected);
          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(event.name.c_str());
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
      if (ImGui::Button("Add", widgetSize))
      {
        int id{};
        anm2.event_add(id);
        selection = {id};
      }
      imgui::set_item_tooltip_shortcut("Add an event.", settings.shortcutAdd);
      ImGui::SameLine();

      std::set<int> unusedEventIDs = anm2.events_unused();

      imgui::shortcut(settings.shortcutRemove, true);
      ImGui::BeginDisabled(unusedEventIDs.empty());
      {
        if (ImGui::Button("Remove Unused", widgetSize))
          for (auto& id : unusedEventIDs)
            anm2.content.layers.erase(id);
      }
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused events (i.e., ones not used by any trigger in any animation.)",
                                       settings.shortcutRemove);
    }
    ImGui::End();
  }
}
