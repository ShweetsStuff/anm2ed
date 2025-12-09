#include "nulls.h"

#include <ranges>

#include "log.h"
#include "map_.h"
#include "strings.h"
#include "toast.h"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace anm2ed::types;

namespace anm2ed::imgui
{
  void Nulls::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.null.reference;
    auto& unused = document.null.unused;
    auto& hovered = document.null.hovered;
    auto& selection = document.null.selection;
    auto& propertiesPopup = manager.nullPropertiesPopup;

    hovered = -1;

    if (ImGui::Begin(localize.get(LABEL_NULLS_WINDOW), &settings.windowIsNulls))
    {
      auto childSize = size_without_footer_get();

      if (ImGui::BeginChild("##Nulls Child", childSize, true))
      {
        selection.start(anm2.content.nulls.size());

        for (auto& [id, null] : anm2.content.nulls)
        {
          auto isSelected = selection.contains(id);
          auto isReferenced = reference == id;

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          ImGui::Selectable(std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, null.name)).c_str(),
                            isSelected);
          if (newNullId == id)
          {
            ImGui::SetScrollHereY(0.5f);
            newNullId = -1;
          }
          if (ImGui::IsItemHovered())
          {
            hovered = id;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) manager.null_properties_open(id);
          }

          if (isReferenced) ImGui::PopFont();

          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(null.name.c_str());
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
              clipboardText += anm2.content.nulls[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.nulls[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          std::string errorString{};
          document.snapshot(localize.get(EDIT_PASTE_NULLS));
          if (anm2.nulls_deserialize(clipboard.get(), type, &errorString))
            document.change(Document::NULLS);
          else
          {
            toasts.push(std::vformat(localize.get(TOAST_DESERIALIZE_NULLS_FAILED), std::make_format_args(errorString)));
            logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_NULLS_FAILED, anm2ed::ENGLISH),
                                      std::make_format_args(errorString)));
          }
        };

        if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste(merge::APPEND);

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::MenuItem(localize.get(BASIC_CUT), settings.shortcutCut.c_str(), false, false);
          if (ImGui::MenuItem(localize.get(BASIC_COPY), settings.shortcutCopy.c_str(), false,
                              selection.empty() || hovered > -1))
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
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize)) manager.null_properties_open();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_NULL), settings.shortcutAdd);
      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_REMOVE]);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), widgetSize))
      {
        auto remove_unused = [&]()
        {
          for (auto& id : unused)
            anm2.content.nulls.erase(id);
          unused.clear();
        };

        DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_NULLS), Document::EVENTS, remove_unused());
      }
      ImGui::EndDisabled();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_NULLS), settings.shortcutRemove);
    }
    ImGui::End();

    manager.null_properties_trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label(), &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = child_size_get(2);
      auto& null = manager.editNull;

      if (ImGui::BeginChild("##Child", childSize, ImGuiChildFlags_Borders))
      {
        if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string(localize.get(BASIC_NAME), &null.name);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_NAME));

        ImGui::Checkbox(localize.get(LABEL_RECT), &null.isShowRect);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_RECT));
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(reference == -1 ? localize.get(BASIC_ADD) : localize.get(BASIC_CONFIRM), widgetSize))
      {
        if (reference == -1)
        {
          auto add = [&]()
          {
            auto id = map::next_id_get(anm2.content.nulls);
            anm2.content.nulls[id] = null;
            selection = {id};
            newNullId = id;
          };

          DOCUMENT_EDIT(document, localize.get(EDIT_ADD_NULL), Document::NULLS, add());
        }
        else
        {
          auto set = [&]()
          {
            anm2.content.nulls[reference] = null;
            selection = {reference};
          };

          DOCUMENT_EDIT(document, localize.get(EDIT_SET_NULL_PROPERTIES), Document::NULLS, set());
        }

        manager.null_properties_close();
      }

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) manager.null_properties_close();

      ImGui::EndPopup();
    }

    manager.null_properties_end();
  }
}
