#include "events.h"

#include <ranges>

using namespace anm2ed::clipboard;
using namespace anm2ed::manager;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;

namespace anm2ed::events
{
  void Events::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& unused = document.unusedEventIDs;
    auto& hovered = document.hoveredEvent;
    auto& multiSelect = document.eventMultiSelect;

    if (ImGui::Begin("Events", &settings.windowIsEvents))
    {
      auto childSize = imgui::size_without_footer_get();
      bool isRenamed{};

      if (ImGui::BeginChild("##Events Child", childSize, true))
      {
        multiSelect.start(anm2.content.events.size());

        for (auto& [id, event] : anm2.content.events)
        {
          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          if (imgui::selectable_input_text(event.name, std::format("###Document #{} Event #{}", manager.selected, id),
                                           event.name, multiSelect.contains(id), 0, &isRenamed))
            if (ImGui::IsItemHovered()) hovered = id;
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

        multiSelect.finish();

        auto copy = [&]()
        {
          if (!multiSelect.empty())
          {
            std::string clipboardText{};
            for (auto& id : multiSelect)
              clipboardText += anm2.content.events[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.events[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          auto clipboardText = clipboard.get();
          document.events_deserialize(clipboardText, type);
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
      if (ImGui::Button("Add", widgetSize)) document.event_add();
      imgui::set_item_tooltip_shortcut("Add an event.", settings.shortcutAdd);
      ImGui::SameLine();

      imgui::shortcut(settings.shortcutRemove);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button("Remove Unused", widgetSize)) document.events_remove_unused();
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused events (i.e., ones not used by any trigger in any animation.)",
                                       settings.shortcutRemove);
    }
    ImGui::End();
  }
}
