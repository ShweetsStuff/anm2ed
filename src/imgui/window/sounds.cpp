#include "sounds.h"

#include <ranges>

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

    if (ImGui::Begin("Sounds", &settings.windowIsSounds))
    {
      auto childSize = imgui::size_without_footer_get();

      if (ImGui::BeginChild("##Sounds Child", childSize, true))
      {
        selection.start(anm2.content.sounds.size());

        for (auto& [id, sound] : anm2.content.sounds)
        {
          auto isSelected = selection.contains(id);
          auto isReferenced = reference == id;

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          if (ImGui::Selectable(sound.path.c_str(), isSelected)) sound.play();
          if (ImGui::IsItemHovered()) hovered = id;

          if (isReferenced) ImGui::PopFont();

          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(sound.path.c_str());
            ImGui::PopFont();
            ImGui::Text("ID: %d", id);
            ImGui::Text("Click to play.");
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
          document.snapshot("Paste Sound(s)");
          if (anm2.sounds_deserialize(clipboard.get(), document.directory_get().string(), type, &errorString))
            document.change(Document::SOUNDS);
          else
            toasts.error(std::format("Failed to deserialize sound(s): {}", errorString));
        };

        if (imgui::shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (imgui::shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste(merge::APPEND);

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::MenuItem("Cut", settings.shortcutCut.c_str(), false, false);
          if (ImGui::MenuItem("Copy", settings.shortcutCopy.c_str(), !selection.empty() && hovered > -1)) copy();

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

      auto widgetSize = imgui::widget_size_with_row_get(2);

      imgui::shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button("Add", widgetSize)) dialog.file_open(dialog::SOUND_OPEN);
      imgui::set_item_tooltip_shortcut("Add a sound.", settings.shortcutAdd);
      ImGui::SameLine();

      imgui::shortcut(manager.chords[SHORTCUT_REMOVE]);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button("Remove Unused", widgetSize))
      {
        auto remove_unused = [&]()
        {
          for (auto& id : unused)
            anm2.content.sounds.erase(id);
          unused.clear();
        };

        DOCUMENT_EDIT(document, "Remove Unused Sounds", Document::SOUNDS, remove_unused());
      };
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused sounds (i.e., ones not used in any trigger.)",
                                       settings.shortcutRemove);
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
          toasts.info(std::format("Initialized sound #{}: {}", id, dialog.path));
        }
        else
          toasts.error(std::format("Failed to initialize sound: {}", dialog.path));
      };

      DOCUMENT_EDIT(document, "Add Sound", Document::SOUNDS, add());

      dialog.reset();
    }
  }
}
