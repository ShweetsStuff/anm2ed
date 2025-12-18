#include "sounds.h"

#include <ranges>

#include "log.h"
#include "path_.h"
#include "strings.h"
#include "toast.h"

using namespace anm2ed::util;
using namespace anm2ed::types;
using namespace anm2ed::resource;
using namespace glm;

namespace anm2ed::imgui
{
  void Sounds::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.sound.reference;
    auto& unused = document.sound.unused;
    auto& selection = document.sound.selection;
    auto style = ImGui::GetStyle();

    auto add_open = [&]() { dialog.file_open(Dialog::SOUND_OPEN); };
    auto replace_open = [&]() { dialog.file_open(Dialog::SOUND_REPLACE); };

    auto play = [&](anm2::Sound& sound) { sound.play(); };

    auto add = [&](const std::filesystem::path& path)
    {
      auto behavior = [&]()
      {
        int id{};
        auto pathString = path::to_utf8(path);
        if (anm2.sound_add(document.directory_get(), path, id))
        {
          selection = {id};
          newSoundId = id;
          toasts.push(std::vformat(localize.get(TOAST_SOUND_INITIALIZED), std::make_format_args(id, pathString)));
          logger.info(std::vformat(localize.get(TOAST_SOUND_INITIALIZED, anm2ed::ENGLISH),
                                   std::make_format_args(id, pathString)));
        }
        else
        {
          toasts.push(std::vformat(localize.get(TOAST_SOUND_INITIALIZE_FAILED), std::make_format_args(pathString)));
          logger.error(std::vformat(localize.get(TOAST_SOUND_INITIALIZE_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(pathString)));
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_ADD_SOUND), Document::SOUNDS, behavior());
    };

    auto remove_unused = [&]()
    {
      if (unused.empty()) return;

      auto behavior = [&]()
      {
        for (auto& id : unused)
          anm2.content.sounds.erase(id);
        unused.clear();
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_SOUNDS), Document::SOUNDS, behavior());
    };

    auto reload = [&]()
    {
      auto behavior = [&]()
      {
        for (auto& id : selection)
        {
          anm2::Sound& sound = anm2.content.sounds[id];
          sound.reload(document.directory_get());
          auto pathString = path::to_utf8(sound.path);
          toasts.push(std::vformat(localize.get(TOAST_RELOAD_SOUND), std::make_format_args(id, pathString)));
          logger.info(
              std::vformat(localize.get(TOAST_RELOAD_SOUND, anm2ed::ENGLISH), std::make_format_args(id, pathString)));
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_RELOAD_SOUNDS), Document::SOUNDS, behavior());
    };

    auto replace = [&](const std::filesystem::path& path)
    {
      if (selection.size() != 1 || path.empty()) return;

      auto behavior = [&]()
      {
        auto& id = *selection.begin();
        anm2::Sound& sound = anm2.content.sounds[id];
        sound = anm2::Sound(document.directory_get(), path);
        auto pathString = path::to_utf8(sound.path);
        toasts.push(std::vformat(localize.get(TOAST_REPLACE_SOUND), std::make_format_args(id, pathString)));
        logger.info(
            std::vformat(localize.get(TOAST_REPLACE_SOUND, anm2ed::ENGLISH), std::make_format_args(id, pathString)));
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_REPLACE_SOUND), Document::SOUNDS, behavior());
    };

    auto open_directory = [&](anm2::Sound& sound)
    {
      if (sound.path.empty()) return;
      std::error_code ec{};
      auto absolutePath = std::filesystem::weakly_canonical(document.directory_get() / sound.path, ec);
      if (ec) absolutePath = document.directory_get() / sound.path;
      auto target = std::filesystem::is_directory(absolutePath)                 ? absolutePath
                    : std::filesystem::is_directory(absolutePath.parent_path()) ? absolutePath.parent_path()
                                                                                : document.directory_get();
      dialog.file_explorer_open(target);
    };

    auto copy = [&]()
    {
      if (selection.empty()) return;

      std::string clipboardText{};
      for (auto& id : selection)
        clipboardText += anm2.content.sounds[id].to_string(id);
      clipboard.set(clipboardText);
    };

    auto paste = [&]()
    {
      if (clipboard.is_empty()) return;

      auto behavior = [&]()
      {
        std::string errorString{};
        document.snapshot(localize.get(TOAST_SOUNDS_PASTE));
        if (anm2.sounds_deserialize(clipboard.get(), document.directory_get(), merge::APPEND, &errorString))
          document.change(Document::SOUNDS);
        else
        {
          toasts.push(std::vformat(localize.get(TOAST_SOUNDS_DESERIALIZE_ERROR), std::make_format_args(errorString)));
          logger.error(std::vformat(localize.get(TOAST_SOUNDS_DESERIALIZE_ERROR, anm2ed::ENGLISH),
                                    std::make_format_args(errorString)));
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_PASTE_SOUNDS), Document::SOUNDS, behavior());
    };

    auto context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##Sound Context Menu");

      if (ImGui::BeginPopup("##Sound Context Menu"))
      {
        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_UNDO), settings.shortcutUndo.c_str(), false,
                            document.is_able_to_undo()))
          document.undo();

        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_REDO), settings.shortcutRedo.c_str(), false,
                            document.is_able_to_redo()))
          document.redo();

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(LABEL_PLAY), nullptr, false, selection.size() == 1))
          play(anm2.content.sounds[*selection.begin()]);
        if (ImGui::MenuItem(localize.get(BASIC_OPEN_DIRECTORY), nullptr, false, selection.size() == 1))
          open_directory(anm2.content.sounds[*selection.begin()]);

        if (ImGui::MenuItem(localize.get(BASIC_ADD), settings.shortcutAdd.c_str())) add_open();
        if (ImGui::MenuItem(localize.get(BASIC_REMOVE_UNUSED), settings.shortcutRemove.c_str(), false, !unused.empty()))
          remove_unused();

        if (ImGui::MenuItem(localize.get(BASIC_RELOAD), nullptr, false, !selection.empty())) reload();
        if (ImGui::MenuItem(localize.get(BASIC_REPLACE), nullptr, false, selection.size() == 1)) replace_open();

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(BASIC_COPY), settings.shortcutCopy.c_str(), false, !selection.empty())) copy();
        if (ImGui::MenuItem(localize.get(BASIC_PASTE), settings.shortcutPaste.c_str(), false, !clipboard.is_empty()))
          paste();

        ImGui::EndPopup();
      }
      ImGui::PopStyleVar(2);
    };

    if (ImGui::Begin(localize.get(LABEL_SOUNDS_WINDOW), &settings.windowIsSounds))
    {
      auto childSize = imgui::size_without_footer_get();

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
      if (ImGui::BeginChild("##Sounds Child", childSize, ImGuiChildFlags_Borders))
      {
        auto soundChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 2);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

        selection.start(anm2.content.sounds.size());

        for (auto& [id, sound] : anm2.content.sounds)
        {
          ImGui::PushID(id);

          if (ImGui::BeginChild("##Sound Child", soundChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = selection.contains(id);
            auto cursorPos = ImGui::GetCursorPos();
            bool isValid = sound.is_valid();
            auto& soundIcon = isValid ? resources.icons[icon::SOUND] : resources.icons[icon::NONE];
            auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            auto pathString = path::to_utf8(sound.path);

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (ImGui::Selectable("##Sound Selectable", isSelected, 0, soundChildSize))
            {
              reference = id;
              play(sound);
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) open_directory(sound);
            if (newSoundId == id)
            {
              ImGui::SetScrollHereY(0.5f);
              newSoundId = -1;
            }

            auto textWidth = ImGui::CalcTextSize(pathString.c_str()).x;
            auto tooltipPadding = style.WindowPadding.x * 4.0f;
            auto minWidth = textWidth + style.ItemSpacing.x + tooltipPadding;

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
            ImGui::SetNextWindowSize(ImVec2(minWidth, 0), ImGuiCond_Appearing);
            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
              ImGui::TextUnformatted(pathString.c_str());
              ImGui::PopFont();
              ImGui::Text("%s: %d", localize.get(BASIC_ID), id);
              if (!isValid)
              {
                ImGui::Spacing();
                ImGui::TextWrapped("%s", localize.get(TOOLTIP_SOUND_INVALID));
              }
              else
              {
                ImGui::Text("%s", localize.get(TEXT_SOUND_PLAY));
                ImGui::Text("%s", localize.get(TEXT_OPEN_DIRECTORY));
              }
              ImGui::EndTooltip();
            }
            ImGui::PopStyleVar(2);

            ImGui::SetCursorPos(cursorPos);
            auto imageSize = to_imvec2(vec2(soundChildSize.y));
            ImGui::ImageWithBg(soundIcon.id, imageSize, ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);

            ImGui::SetCursorPos(ImVec2(soundChildSize.y + style.ItemSpacing.x,
                                       soundChildSize.y - soundChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));

            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_SOUND), std::make_format_args(id, pathString)).c_str());
          }

          ImGui::EndChild();
          ImGui::PopID();

          context_menu();
        }

        ImGui::PopStyleVar();
        selection.finish();
      }

      ImGui::EndChild();
      ImGui::PopStyleVar();

      auto widgetSize = imgui::widget_size_with_row_get(4);

      imgui::shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize)) add_open();
      imgui::set_item_tooltip_shortcut(localize.get(TOOLTIP_SOUND_ADD), settings.shortcutAdd);

      if (dialog.is_selected(Dialog::SOUND_OPEN))
      {
        add(dialog.path);
        dialog.reset();
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(unused.empty());
      imgui::shortcut(manager.chords[SHORTCUT_REMOVE]);
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), widgetSize)) remove_unused();
      imgui::set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_SOUNDS), settings.shortcutRemove);
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      if (ImGui::Button(localize.get(BASIC_RELOAD), widgetSize)) reload();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RELOAD_SOUNDS));
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.size() != 1);
      if (ImGui::Button(localize.get(BASIC_REPLACE), widgetSize)) replace_open();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REPLACE_SOUND));
      ImGui::EndDisabled();

      if (dialog.is_selected(Dialog::SOUND_REPLACE))
      {
        replace(dialog.path);
        dialog.reset();
      }
    }
    ImGui::End();

    if (imgui::shortcut(manager.chords[SHORTCUT_ADD], shortcut::FOCUSED)) add_open();
    if (imgui::shortcut(manager.chords[SHORTCUT_REMOVE], shortcut::FOCUSED)) remove_unused();
    if (imgui::shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
    if (imgui::shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();
  }
}
