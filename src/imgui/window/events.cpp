#include "events.h"

#include <ranges>

using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui
{
  void Events::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& unused = document.unusedEventIDs;
    auto& hovered = document.hoveredEvent;
    auto& reference = document.referenceEvent;
    auto& multiSelect = document.eventMultiSelect;

    hovered = -1;

    if (ImGui::Begin("Events", &settings.windowIsEvents))
    {
      auto childSize = size_without_footer_get();

      if (ImGui::BeginChild("##Events Child", childSize, true))
      {
        multiSelect.start(anm2.content.events.size());

        for (auto& [id, event] : anm2.content.events)
        {
          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          ImGui::Selectable(event.name.c_str(), multiSelect.contains(id));
          if (ImGui::IsItemHovered())
          {
            hovered = id;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
              reference = id;
              editEvent = document.anm2.content.events[reference];
              propertiesPopup.open();
            }
          }

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

        if (shortcut(settings.shortcutCopy, shortcut::FOCUSED)) copy();
        if (shortcut(settings.shortcutPaste, shortcut::FOCUSED)) paste(merge::APPEND);

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

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(settings.shortcutAdd);
      if (ImGui::Button("Add", widgetSize))
      {
        reference = -1;
        editEvent = anm2::Event();
        propertiesPopup.open();
      }
      set_item_tooltip_shortcut("Add an event.", settings.shortcutAdd);
      ImGui::SameLine();

      shortcut(settings.shortcutRemove);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button("Remove Unused", widgetSize)) document.events_remove_unused();
      ImGui::EndDisabled();
      set_item_tooltip_shortcut("Remove unused events (i.e., ones not used by any trigger in any animation.)",
                                settings.shortcutRemove);
    }
    ImGui::End();

    propertiesPopup.trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label, &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = child_size_get(2);
      auto& event = editEvent;

      if (ImGui::BeginChild("Child", childSize, ImGuiChildFlags_Borders))
      {
        if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string("Name", &event.name);
        ImGui::SetItemTooltip("Set the event's name.");
        combo_strings("Sound", &event.soundID, document.soundNames);
        ImGui::SetItemTooltip("Set the event sound; it will play when a trigger associated with this event activates.");
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(reference == -1 ? "Add" : "Confirm", widgetSize))
      {
        document.event_set(event);
        propertiesPopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) propertiesPopup.close();

      propertiesPopup.end();
      ImGui::EndPopup();
    }
  }
}
