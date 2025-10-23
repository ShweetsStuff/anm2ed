#include "events.h"

#include <ranges>

using namespace anm2ed::document_manager;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;

namespace anm2ed::events
{
  void Events::update(DocumentManager& manager, Settings& settings, Resources& resources)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& selection = document.selectedEvents;

    if (document.is_just_changed(change::EVENTS)) unusedEventIDs = anm2.events_unused();

    storage.user_data_set(&selection);

    if (ImGui::Begin("Events", &settings.windowIsEvents))
    {
      auto childSize = imgui::size_without_footer_get();
      bool isRenamed{};

      if (ImGui::BeginChild("##Events Child", childSize, true))
      {
        storage.begin(anm2.content.events.size());

        for (auto& [id, event] : anm2.content.events)
        {
          auto isSelected = selection.contains(id);

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          if (imgui::selectable_input_text(event.name, std::format("###Document #{} Event #{}", manager.selected, id),
                                           event.name, isSelected, 0, &isRenamed))
            if (isRenamed) document.change(change::EVENTS);
          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(event.name.c_str());
            ImGui::PopFont();
            ImGui::EndTooltip();
          }
          ImGui::PopID();
        }

        storage.end();
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      imgui::shortcut(settings.shortcutAdd);
      if (ImGui::Button("Add", widgetSize))
      {
        int id{};
        anm2.event_add(id);
        selection = {id};
        document.change(change::EVENTS);
      }
      imgui::set_item_tooltip_shortcut("Add an event.", settings.shortcutAdd);
      ImGui::SameLine();

      imgui::shortcut(settings.shortcutRemove);
      ImGui::BeginDisabled(unusedEventIDs.empty());
      {
        if (ImGui::Button("Remove Unused", widgetSize))
        {
          for (auto& id : unusedEventIDs)
            anm2.content.events.erase(id);
          document.change(change::EVENTS);
          unusedEventIDs.clear();
        }
      }
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused events (i.e., ones not used by any trigger in any animation.)",
                                       settings.shortcutRemove);
    }
    ImGui::End();
  }
}
