#include "sounds.h"

#include <ranges>

#include "log.h"
#include "strings.h"
#include "toast.h"

using namespace anm2ed::dialog;
using namespace anm2ed::types;
using namespace anm2ed::resource;

namespace anm2ed::imgui
{
  void Sounds::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.sound.reference;
    auto& unused = document.sound.unused;
    auto& hovered = document.null.hovered;
    auto& selection = document.sound.selection;

    hovered = -1;

    if (ImGui::Begin(localize.get(LABEL_SOUNDS_WINDOW), &settings.windowIsSounds))
    {
      auto childSize = imgui::size_without_footer_get();

      if (ImGui::BeginChild("##Sounds Child", childSize, true))
      {
        selection.start(anm2.content.sounds.size());

        for (auto& [id, sound] : anm2.content.sounds)
        {
          auto isSelected = selection.contains(id);
          auto isReferenced = reference == id;
          const std::string pathString = sound.path.empty() ? std::string{anm2::NO_PATH} : sound.path.string();
          const char* pathLabel = pathString.c_str();

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          if (ImGui::Selectable(pathLabel, isSelected)) sound.play();
          if (ImGui::IsItemHovered()) hovered = id;
          if (newSoundId == id)
          {
            ImGui::SetScrollHereY(0.5f);
            newSoundId = -1;
          }

          if (isReferenced) ImGui::PopFont();

          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(pathLabel);
            ImGui::PopFont();
            ImGui::Text("%s: %d", localize.get(BASIC_ID), id);
            ImGui::Text("%s", localize.get(TOOLTIP_SOUNDS_PLAY));
            if (!sound.is_valid())
            {
              ImGui::Spacing();
              ImGui::TextWrapped("%s", localize.get(TOOLTIP_SOUND_INVALID));
            }
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
              clipboardText += anm2.content.sounds[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.sounds[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          std::string errorString{};
          document.snapshot(localize.get(TOAST_SOUNDS_PASTE));
          if (anm2.sounds_deserialize(clipboard.get(), document.directory_get().string(), type, &errorString))
            document.change(Document::SOUNDS);
          else
          {
            toasts.push(std::vformat(localize.get(TOAST_SOUNDS_DESERIALIZE_ERROR), std::make_format_args(errorString)));
            logger.error(std::vformat(localize.get(TOAST_SOUNDS_DESERIALIZE_ERROR, anm2ed::ENGLISH),
                                      std::make_format_args(errorString)));
          }
        };

        if (imgui::shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (imgui::shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste(merge::APPEND);

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::MenuItem(localize.get(BASIC_CUT), settings.shortcutCut.c_str(), false, false);
          if (ImGui::MenuItem(localize.get(BASIC_COPY), settings.shortcutCopy.c_str(),
                              !selection.empty() && hovered > -1))
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

      auto widgetSize = imgui::widget_size_with_row_get(2);

      imgui::shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize)) dialog.file_open(dialog::SOUND_OPEN);
      imgui::set_item_tooltip_shortcut(localize.get(TOOLTIP_SOUND_ADD), settings.shortcutAdd);
      ImGui::SameLine();

      imgui::shortcut(manager.chords[SHORTCUT_REMOVE]);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), widgetSize))
      {
        auto remove_unused = [&]()
        {
          for (auto& id : unused)
            anm2.content.sounds.erase(id);
          unused.clear();
        };

        DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_SOUNDS), Document::SOUNDS, remove_unused());
      };
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_SOUNDS), settings.shortcutRemove);
    }
    ImGui::End();

    if (dialog.is_selected(dialog::SOUND_OPEN))
    {
      auto add = [&]()
      {
        int id{};
        if (anm2.sound_add(document.directory_get().string(), dialog.path, id))
        {
          selection = {id};
          newSoundId = id;
          toasts.push(std::vformat(localize.get(TOAST_SOUND_INITIALIZED), std::make_format_args(id, dialog.path)));
          logger.info(std::vformat(localize.get(TOAST_SOUND_INITIALIZED, anm2ed::ENGLISH),
                                   std::make_format_args(id, dialog.path)));
        }
        else
        {
          toasts.push(std::vformat(localize.get(TOAST_SOUND_INITIALIZE_FAILED), std::make_format_args(dialog.path)));
          logger.error(std::vformat(localize.get(TOAST_SOUND_INITIALIZE_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(dialog.path)));
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_ADD_SOUND), Document::SOUNDS, add());

      dialog.reset();
    }
  }
}
