#include "events.h"

#include <ranges>

#include "map_.h"
#include "toast.h"

using namespace anm2ed::util;
using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui
{
  void Events::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& unused = document.event.unused;
    auto& hovered = document.event.hovered;
    auto& reference = document.event.reference;
    auto& selection = document.event.selection;

    hovered = -1;

    if (ImGui::Begin("Events", &settings.windowIsEvents))
    {
      auto childSize = size_without_footer_get();

      if (ImGui::BeginChild("##Events Child", childSize, true))
      {
        selection.start(anm2.content.events.size());

        for (auto& [id, event] : anm2.content.events)
        {
          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          if (selectable_input_text(event.name, std::format("###Document #{} Event #{}", manager.selected, id),
                                    event.name, selection.contains(id)))
            if (ImGui::IsItemHovered()) hovered = id;

          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(event.name.c_str());
            ImGui::PopFont();
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
              clipboardText += anm2.content.events[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.events[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          std::string errorString{};
          document.snapshot("Paste Event(s)");
          if (anm2.events_deserialize(clipboard.get(), type, &errorString))
            document.change(Document::EVENTS);
          else
            toasts.error(std::format("Failed to deserialize event(s): {}", errorString));
        };

        if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste(merge::APPEND);

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

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button("Add", widgetSize))
      {
        auto add = [&]()
        {
          auto id = map::next_id_get(anm2.content.events);
          anm2.content.events[id] = anm2::Event();
          selection = {id};
          reference = {id};
        };

        DOCUMENT_EDIT(document, "Add Event", Document::EVENTS, add());
      }
      set_item_tooltip_shortcut("Add an event.", settings.shortcutAdd);
      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_REMOVE]);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button("Remove Unused", widgetSize))
      {
        auto remove_unused = [&]()
        {
          for (auto& id : unused)
            anm2.content.events.erase(id);
          unused.clear();
        };

        DOCUMENT_EDIT(document, "Remove Unused Events", Document::EVENTS, remove_unused());
      }
      ImGui::EndDisabled();
      set_item_tooltip_shortcut("Remove unused events (i.e., ones not used by any trigger in any animation.)",
                                settings.shortcutRemove);
    }
    ImGui::End();
  }
}
