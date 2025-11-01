#include "sounds.h"

#include <ranges>

using namespace anm2ed::dialog;
using namespace anm2ed::types;
using namespace anm2ed::resource;

namespace anm2ed::imgui
{
  void Sounds::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.referenceNull;
    auto& unused = document.unusedNullIDs;
    auto& hovered = document.hoveredNull;
    auto& multiSelect = document.soundMultiSelect;

    hovered = -1;

    if (ImGui::Begin("Sounds", &settings.windowIsSounds))
    {
      auto childSize = imgui::size_without_footer_get();

      if (ImGui::BeginChild("##Sounds Child", childSize, true))
      {
        multiSelect.start(anm2.content.sounds.size());

        for (auto& [id, sound] : anm2.content.sounds)
        {
          auto isSelected = multiSelect.contains(id);
          auto isReferenced = reference == id;

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          if (ImGui::Selectable(std::format(anm2::SOUND_FORMAT, id, sound.path.string()).c_str(), isSelected))
            sound.audio.play();
          if (ImGui::IsItemHovered())
          {
            hovered = id;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
              ;
          }

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

        multiSelect.finish();

        auto copy = [&]()
        {
          if (!multiSelect.empty())
          {
            std::string clipboardText{};
            for (auto& id : multiSelect)
              clipboardText += anm2.content.sounds[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.sounds[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          auto clipboardText = clipboard.get();
          document.sounds_deserialize(clipboardText, type);
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
      if (ImGui::Button("Add", widgetSize)) dialog.file_open(dialog::SOUND_OPEN);
      imgui::set_item_tooltip_shortcut("Add a sound.", settings.shortcutAdd);
      ImGui::SameLine();

      imgui::shortcut(settings.shortcutRemove);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button("Remove Unused", widgetSize))
        ;
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Remove unused sounds (i.e., ones not used in any trigger.)",
                                       settings.shortcutRemove);
    }
    ImGui::End();

    if (dialog.is_selected(dialog::SOUND_OPEN))
    {
      document.sound_add(dialog.path);
      dialog.reset();
    }
  }
}