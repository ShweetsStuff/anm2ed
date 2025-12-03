#include "events.h"

#include <ranges>

#include "log.h"
#include "map_.h"
#include "strings.h"
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

    if (ImGui::Begin(localize.get(LABEL_EVENTS_WINDOW), &settings.windowIsEvents))
    {
      auto childSize = size_without_footer_get();

      if (ImGui::BeginChild("##Events Child", childSize, true))
      {
        selection.start(anm2.content.events.size());

        for (auto& [id, event] : anm2.content.events)
        {
          auto isNewEvent = (newEventId == id);

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          if (isNewEvent) renameState = RENAME_FORCE_EDIT;
          if (selectable_input_text(event.name, std::format("###Document #{} Event #{}", manager.selected, id),
                                    event.name, selection.contains(id), ImGuiSelectableFlags_None, renameState))
          {
            if (renameState == RENAME_BEGIN)
              document.snapshot(localize.get(EDIT_RENAME_EVENT));
            else if (renameState == RENAME_FINISHED)
              document.change(Document::EVENTS);
          }
          if (ImGui::IsItemHovered()) hovered = id;

          if (isNewEvent)
          {
            ImGui::SetScrollHereY(0.5f);
            newEventId = -1;
          }

          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(event.name.c_str());
            ImGui::PopFont();
            ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
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
          document.snapshot(localize.get(EDIT_PASTE_EVENTS));
          if (anm2.events_deserialize(clipboard.get(), type, &errorString))
            document.change(Document::EVENTS);
          else
          {
            toasts.push(std::vformat(localize.get(TOAST_DESERIALIZE_EVENTS_FAILED),
                                     std::make_format_args(errorString)));
            logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_EVENTS_FAILED, anm2ed::ENGLISH),
                                      std::make_format_args(errorString)));
          }
        };

        if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste(merge::APPEND);

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::MenuItem(localize.get(BASIC_CUT), settings.shortcutCut.c_str(), false, false);
          if (ImGui::MenuItem(localize.get(BASIC_COPY), settings.shortcutCopy.c_str(), false,
                              !selection.empty() || hovered > -1))
            copy();

          if (ImGui::BeginMenu(localize.get(BASIC_PASTE), !clipboard.is_empty()))
          {
            if (ImGui::MenuItem(localize.get(BASIC_APPEND), settings.shortcutPaste.c_str())) paste(merge::APPEND);
            if (ImGui::MenuItem(localize.get(BASIC_REPLACE))) paste(merge::REPLACE);

            ImGui::EndMenu();
          }

          ImGui::EndPopup();
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize))
      {
        auto add = [&]()
        {
          auto id = map::next_id_get(anm2.content.events);
          anm2::Event event{};
          event.name = localize.get(TEXT_NEW_EVENT);
          anm2.content.events[id] = event;
          selection = {id};
          reference = {id};
          newEventId = id;
        };

        DOCUMENT_EDIT(document, localize.get(EDIT_ADD_EVENT), Document::EVENTS, add());
      }
      set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_EVENT), settings.shortcutAdd);
      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_REMOVE]);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), widgetSize))
      {
        auto remove_unused = [&]()
        {
          for (auto& id : unused)
            anm2.content.events.erase(id);
          unused.clear();
        };

        DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_EVENTS), Document::EVENTS, remove_unused());
      }
      ImGui::EndDisabled();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_EVENTS), settings.shortcutRemove);
    }
    ImGui::End();
  }
}
