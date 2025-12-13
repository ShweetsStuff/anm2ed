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
    auto& reference = document.event.reference;
    auto& selection = document.event.selection;

    auto rename_format_get = [&](int id) { return std::format("###Document #{} Event #{}", manager.selected, id); };
    auto rename = [&]()
    {
      if (!selection.empty()) renameQueued = *selection.begin();
    };

    auto add = [&]()
    {
      auto behavior = [&]()
      {
        auto id = map::next_id_get(anm2.content.events);
        anm2::Event event{};
        event.name = localize.get(TEXT_NEW_EVENT);
        anm2.content.events[id] = event;
        selection = {id};
        reference = {id};
        newEventId = id;
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_ADD_EVENT), Document::EVENTS, behavior());
    };

    auto remove_unused = [&]()
    {
      if (unused.empty()) return;

      auto behavior = [&]()
      {
        for (auto& id : unused)
          anm2.content.events.erase(id);
        unused.clear();
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_EVENTS), Document::EVENTS, behavior());
    };

    auto copy = [&]()
    {
      if (selection.empty()) return;

      std::string clipboardText{};
      for (auto& id : selection)
        clipboardText += anm2.content.events[id].to_string(id);
      clipboard.set(clipboardText);
    };

    auto paste = [&]()
    {
      if (clipboard.is_empty()) return;

      auto behavior = [&]()
      {
        std::string errorString{};
        document.snapshot(localize.get(EDIT_PASTE_EVENTS));
        if (anm2.events_deserialize(clipboard.get(), merge::APPEND, &errorString))
          document.change(Document::EVENTS);
        else
        {
          toasts.push(std::vformat(localize.get(TOAST_DESERIALIZE_EVENTS_FAILED), std::make_format_args(errorString)));
          logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_EVENTS_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(errorString)));
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_PASTE_EVENTS), Document::EVENTS, behavior());
    };

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
          if (isNewEvent || renameQueued == id)
          {
            renameState = RENAME_FORCE_EDIT;
            renameQueued = -1;
          }
          if (selectable_input_text(event.name, rename_format_get(id), event.name, selection.contains(id),
                                    ImGuiSelectableFlags_None, renameState))
          {
            if (renameState == RENAME_BEGIN)
              document.snapshot(localize.get(EDIT_RENAME_EVENT));
            else if (renameState == RENAME_FINISHED)
              document.change(Document::EVENTS);
          }

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

        if (shortcut(manager.chords[SHORTCUT_RENAME], shortcut::FOCUSED)) rename();
        if (shortcut(manager.chords[SHORTCUT_ADD], shortcut::FOCUSED)) add();
        if (shortcut(manager.chords[SHORTCUT_REMOVE], shortcut::FOCUSED)) remove_unused();
        if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_UNDO), settings.shortcutUndo.c_str(), false,
                              document.is_able_to_undo()))
            document.undo();

          if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_REDO), settings.shortcutRedo.c_str(), false,
                              document.is_able_to_redo()))
            document.redo();

          ImGui::Separator();

          if (ImGui::MenuItem(localize.get(BASIC_RENAME), settings.shortcutRename.c_str(), false,
                              selection.size() == 1))
            rename();
          if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_ADD), settings.shortcutAdd.c_str())) add();
          if (ImGui::MenuItem(localize.get(BASIC_REMOVE_UNUSED), settings.shortcutRemove.c_str(), false,
                              !unused.empty()))
            remove_unused();

          ImGui::Separator();

          if (ImGui::MenuItem(localize.get(BASIC_COPY), settings.shortcutCopy.c_str(), false, !selection.empty()))
            copy();

          if (ImGui::MenuItem(localize.get(BASIC_PASTE), settings.shortcutPaste.c_str(), false, !clipboard.is_empty()))
            paste();

          ImGui::EndPopup();
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize)) add();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_EVENT), settings.shortcutAdd);
      ImGui::SameLine();

      ImGui::BeginDisabled(unused.empty());
      shortcut(manager.chords[SHORTCUT_REMOVE]);
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), widgetSize)) remove_unused();
      ImGui::EndDisabled();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_EVENTS), settings.shortcutRemove);
    }
    ImGui::End();
  }
}
