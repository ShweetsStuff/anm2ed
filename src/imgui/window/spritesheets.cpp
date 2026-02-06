#include "spritesheets.h"

#include <ranges>

#include <filesystem>
#include <format>
#include <functional>

#include "document.h"
#include "log.h"
#include "path_.h"
#include "strings.h"
#include "toast.h"

using namespace anm2ed::types;
using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  void Spritesheets::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog,
                            Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& selection = document.spritesheet.selection;
    auto& reference = document.spritesheet.reference;
    auto& region = document.region;
    auto style = ImGui::GetStyle();
    std::function<void()> pack{};

    auto add_open = [&]() { dialog.file_open(Dialog::SPRITESHEET_OPEN); };
    auto replace_open = [&]() { dialog.file_open(Dialog::SPRITESHEET_REPLACE); };
    auto merge_open = [&]()
    {
      if (selection.size() <= 1) return;
      mergeSelection = selection;
      mergePopup.open();
    };
    auto pack_open = [&]()
    {
      if (selection.size() != 1) return;
      auto id = *selection.begin();
      if (!anm2.content.spritesheets.contains(id)) return;
      if (anm2.content.spritesheets.at(id).regions.empty()) return;
      if (pack) pack();
    };

    auto add = [&](const std::filesystem::path& path)
    {
      if (path.empty()) return;
      document.spritesheet_add(path);
      newSpritesheetId = document.spritesheet.reference;
    };

    auto remove_unused = [&]()
    {
      auto unused = anm2.spritesheets_unused();
      if (unused.empty()) return;

      auto behavior = [&]()
      {
        for (auto& id : unused)
        {
          anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
          auto pathString = path::to_utf8(spritesheet.path);
          toasts.push(std::vformat(localize.get(TOAST_REMOVE_SPRITESHEET), std::make_format_args(id, pathString)));
          logger.info(std::vformat(localize.get(TOAST_REMOVE_SPRITESHEET, anm2ed::ENGLISH),
                                   std::make_format_args(id, pathString)));
          anm2.content.spritesheets.erase(id);
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_SPRITESHEETS), Document::ALL, behavior());
    };

    auto reload = [&]()
    {
      if (selection.empty()) return;

      auto behavior = [&]()
      {
        for (auto& id : selection)
        {
          anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
          spritesheet.reload(document.directory_get());
          auto pathString = path::to_utf8(spritesheet.path);
          toasts.push(std::vformat(localize.get(TOAST_RELOAD_SPRITESHEET), std::make_format_args(id, pathString)));
          logger.info(std::vformat(localize.get(TOAST_RELOAD_SPRITESHEET, anm2ed::ENGLISH),
                                   std::make_format_args(id, pathString)));
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_RELOAD_SPRITESHEETS), Document::SPRITESHEETS, behavior());
    };

    auto replace = [&](const std::filesystem::path& path)
    {
      if (selection.size() != 1 || path.empty()) return;

      auto behavior = [&]()
      {
        auto& id = *selection.begin();
        anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
        spritesheet.reload(document.directory_get(), path);
        auto pathString = path::to_utf8(spritesheet.path);
        toasts.push(std::vformat(localize.get(TOAST_REPLACE_SPRITESHEET), std::make_format_args(id, pathString)));
        logger.info(std::vformat(localize.get(TOAST_REPLACE_SPRITESHEET, anm2ed::ENGLISH),
                                 std::make_format_args(id, pathString)));
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_REPLACE_SPRITESHEET), Document::SPRITESHEETS, behavior());
    };

    auto save = [&]()
    {
      if (selection.empty()) return;

      for (auto& id : selection)
      {
        anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
        auto pathString = path::to_utf8(spritesheet.path);
        if (spritesheet.save(document.directory_get()))
        {
          toasts.push(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET), std::make_format_args(id, pathString)));
          logger.info(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET, anm2ed::ENGLISH),
                                   std::make_format_args(id, pathString)));
        }
        else
        {
          toasts.push(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED), std::make_format_args(id, pathString)));
          logger.error(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(id, pathString)));
        }
      }
    };

    auto merge = [&]()
    {
      if (mergeSelection.size() <= 1) return;

      auto behavior = [&]()
      {
        auto baseID = *mergeSelection.begin();
        if (anm2.spritesheets_merge(mergeSelection, (anm2::SpritesheetMergeOrigin)settings.mergeSpritesheetsOrigin,
                                    settings.mergeSpritesheetsIsMakeRegions,
                                    (origin::Type)settings.mergeSpritesheetsRegionOrigin))
        {
          selection = {baseID};
          reference = baseID;
          region.reference = -1;
          region.selection.clear();
          toasts.push(localize.get(TOAST_MERGE_SPRITESHEETS));
          logger.info(localize.get(TOAST_MERGE_SPRITESHEETS, anm2ed::ENGLISH));
        }
        else
        {
          toasts.push(localize.get(TOAST_MERGE_SPRITESHEETS_FAILED));
          logger.error(localize.get(TOAST_MERGE_SPRITESHEETS_FAILED, anm2ed::ENGLISH));
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_MERGE_SPRITESHEETS), Document::ALL, behavior());
    };
    pack = [&]()
    {
      if (selection.size() != 1) return;

      auto behavior = [&]()
      {
        auto id = *selection.begin();
        if (anm2.spritesheet_pack(id))
        {
          toasts.push(localize.get(TOAST_PACK_SPRITESHEET));
          logger.info(localize.get(TOAST_PACK_SPRITESHEET, anm2ed::ENGLISH));
        }
        else
        {
          toasts.push(localize.get(TOAST_PACK_SPRITESHEET_FAILED));
          logger.error(localize.get(TOAST_PACK_SPRITESHEET_FAILED, anm2ed::ENGLISH));
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_PACK_SPRITESHEET), Document::SPRITESHEETS, behavior());
    };

    auto open_directory = [&](anm2::Spritesheet& spritesheet)
    {
      if (spritesheet.path.empty()) return;
      std::error_code ec{};
      auto absolutePath = std::filesystem::weakly_canonical(document.directory_get() / spritesheet.path, ec);
      if (ec) absolutePath = document.directory_get() / spritesheet.path;
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
        clipboardText += anm2.content.spritesheets[id].to_string(id);
      clipboard.set(clipboardText);
    };

    auto paste = [&]()
    {
      if (clipboard.is_empty()) return;

      auto behavior = [&]()
      {
        auto maxSpritesheetIdBefore = anm2.content.spritesheets.empty() ? -1 : anm2.content.spritesheets.rbegin()->first;
        std::string errorString{};
        document.snapshot(localize.get(EDIT_PASTE_SPRITESHEETS));
        if (anm2.spritesheets_deserialize(clipboard.get(), document.directory_get(), merge::APPEND, &errorString))
        {
          if (!anm2.content.spritesheets.empty())
          {
            auto maxSpritesheetIdAfter = anm2.content.spritesheets.rbegin()->first;
            if (maxSpritesheetIdAfter > maxSpritesheetIdBefore)
            {
              newSpritesheetId = maxSpritesheetIdAfter;
              selection = {maxSpritesheetIdAfter};
              reference = maxSpritesheetIdAfter;
              region.reference = -1;
              region.selection.clear();
            }
          }
          document.change(Document::SPRITESHEETS);
        }
        else
        {
          toasts.push(
              std::vformat(localize.get(TOAST_DESERIALIZE_SPRITESHEETS_FAILED), std::make_format_args(errorString)));
          logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_SPRITESHEETS_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(errorString)));
        };
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_PASTE_SPRITESHEETS), Document::SPRITESHEETS, behavior());
    };

    auto context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##Spritesheet Context Menu");

      if (ImGui::BeginPopup("##Spritesheet Context Menu"))
      {
        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_UNDO), settings.shortcutUndo.c_str(), false,
                            document.is_able_to_undo()))
          document.undo();

        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_REDO), settings.shortcutRedo.c_str(), false,
                            document.is_able_to_redo()))
          document.redo();

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(BASIC_OPEN_DIRECTORY), nullptr, false, selection.size() == 1))
          open_directory(anm2.content.spritesheets[*selection.begin()]);

        if (ImGui::MenuItem(localize.get(BASIC_ADD), settings.shortcutAdd.c_str())) add_open();
        if (ImGui::MenuItem(localize.get(BASIC_REMOVE_UNUSED), settings.shortcutRemove.c_str())) remove_unused();

        bool isPackable =
            selection.size() == 1 && anm2.content.spritesheets.contains(*selection.begin()) &&
            !anm2.content.spritesheets.at(*selection.begin()).regions.empty();

        if (ImGui::MenuItem(localize.get(BASIC_RELOAD), nullptr, false, !selection.empty())) reload();
        if (ImGui::MenuItem(localize.get(BASIC_REPLACE), nullptr, false, selection.size() == 1)) replace_open();
        if (ImGui::MenuItem(localize.get(BASIC_MERGE), settings.shortcutMerge.c_str(), false, selection.size() > 1))
          merge_open();
        if (ImGui::MenuItem(localize.get(BASIC_PACK), nullptr, false, isPackable)) pack_open();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PACK_SPRITESHEET));
        if (ImGui::MenuItem(localize.get(BASIC_SAVE), nullptr, false, !selection.empty())) save();

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(BASIC_COPY), settings.shortcutCopy.c_str(), false, !selection.empty())) copy();
        if (ImGui::MenuItem(localize.get(BASIC_PASTE), settings.shortcutPaste.c_str(), false, !clipboard.is_empty()))
          paste();
        ImGui::EndPopup();
      }
      ImGui::PopStyleVar(2);
    };

    if (ImGui::Begin(localize.get(LABEL_SPRITESHEETS_WINDOW), &settings.windowIsSpritesheets))
    {
      auto childSize = size_without_footer_get(2);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
      if (ImGui::BeginChild("##Spritesheets Child", childSize, ImGuiChildFlags_Borders))
      {
        auto spritesheetChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 4);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

        selection.start(anm2.content.spritesheets.size());
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
        {
          selection.clear();
          for (auto& id : anm2.content.spritesheets | std::views::keys)
            selection.insert(id);
        }
        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused)) selection.clear();

        for (auto& [id, spritesheet] : anm2.content.spritesheets)
        {
          auto isNewSpritesheet = newSpritesheetId == id;
          ImGui::PushID(id);

          if (ImGui::BeginChild("##Spritesheet Child", spritesheetChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = selection.contains(id);
            auto isReferenced = id == reference;
            auto cursorPos = ImGui::GetCursorPos();
            bool isValid = spritesheet.texture.is_valid();
            auto& texture = isValid ? spritesheet.texture : resources.icons[icon::NONE];
            auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            auto pathString = path::to_utf8(spritesheet.path);
            auto pathCStr = pathString.c_str();

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (ImGui::Selectable("##Spritesheet Selectable", isSelected, 0, spritesheetChildSize))
            {
              reference = id;
              region.reference = -1;
              region.selection.clear();
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
              open_directory(spritesheet);

            auto viewport = ImGui::GetMainViewport();
            auto maxPreviewSize = to_vec2(viewport->Size) * 0.5f;
            vec2 textureSize = vec2(glm::max(texture.size.x, 1), glm::max(texture.size.y, 1));
            if (textureSize.x > maxPreviewSize.x || textureSize.y > maxPreviewSize.y)
            {
              auto scale = glm::min(maxPreviewSize.x / textureSize.x, maxPreviewSize.y / textureSize.y);
              textureSize *= scale;
            }

            auto textWidth = ImGui::CalcTextSize(pathCStr).x;
            auto tooltipPadding = style.WindowPadding.x * 4.0f;
            auto minWidth = textureSize.x + style.ItemSpacing.x + textWidth + tooltipPadding;

            ImGui::SetNextWindowSize(ImVec2(minWidth, 0), ImGuiCond_Appearing);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
              auto childFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
              auto noScrollFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
              if (ImGui::BeginChild("##Spritesheet Tooltip Image Child", to_imvec2(textureSize), childFlags,
                                    noScrollFlags))
                ImGui::ImageWithBg(texture.id, to_imvec2(textureSize), ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);
              ImGui::EndChild();
              ImGui::PopStyleVar();

              ImGui::SameLine();

              auto infoChildFlags = ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
              if (ImGui::BeginChild("##Spritesheet Info Tooltip Child", ImVec2(), infoChildFlags, noScrollFlags))
              {
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(pathCStr);
                ImGui::PopFont();

                ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());

                if (!isValid)
                  ImGui::TextUnformatted(localize.get(TOOLTIP_SPRITESHEET_INVALID));
                else
                  ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_TEXTURE_SIZE),
                                                      std::make_format_args(texture.size.x, texture.size.y))
                                             .c_str());

                ImGui::TextUnformatted(localize.get(TEXT_OPEN_DIRECTORY));
              }
              ImGui::EndChild();

              ImGui::EndTooltip();
            }
            ImGui::PopStyleVar(2);

            auto imageSize = to_imvec2(vec2(spritesheetChildSize.y));
            auto aspectRatio = (float)texture.size.x / texture.size.y;

            if (imageSize.x / imageSize.y > aspectRatio)
              imageSize.x = imageSize.y * aspectRatio;
            else
              imageSize.y = imageSize.x / aspectRatio;

            ImGui::SetCursorPos(cursorPos);
            ImGui::ImageWithBg(texture.id, imageSize, ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);

            ImGui::SetCursorPos(
                ImVec2(spritesheetChildSize.y + style.ItemSpacing.x,
                       spritesheetChildSize.y - spritesheetChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));

            if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(id, pathCStr)).c_str());
            if (isReferenced) ImGui::PopFont();
          }

          ImGui::EndChild();

          if (isNewSpritesheet)
          {
            ImGui::SetScrollHereY(0.5f);
            newSpritesheetId = -1;
          }

          ImGui::PopID();
        }

        ImGui::PopStyleVar();
        selection.finish();
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();

      context_menu();

      auto rowOneWidgetSize = widget_size_with_row_get(3);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), rowOneWidgetSize)) add_open();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_SPRITESHEET), settings.shortcutAdd);

      if (dialog.is_selected(Dialog::SPRITESHEET_OPEN))
      {
        add(dialog.path);
        dialog.reset();
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      if (ImGui::Button(localize.get(BASIC_RELOAD), rowOneWidgetSize)) reload();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RELOAD_SPRITESHEETS));
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.size() != 1);
      if (ImGui::Button(localize.get(BASIC_REPLACE), rowOneWidgetSize)) replace_open();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REPLACE_SPRITESHEET));
      ImGui::EndDisabled();

      if (dialog.is_selected(Dialog::SPRITESHEET_REPLACE))
      {
        replace(dialog.path);
        dialog.reset();
      }

      auto rowTwoWidgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_REMOVE]);
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), rowTwoWidgetSize)) remove_unused();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_SPRITESHEETS), settings.shortcutRemove);

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      if (ImGui::Button(localize.get(BASIC_SAVE), rowTwoWidgetSize)) save();
      ImGui::EndDisabled();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SAVE_SPRITESHEETS));

      if (imgui::shortcut(manager.chords[SHORTCUT_ADD], shortcut::FOCUSED)) add_open();
      if (imgui::shortcut(manager.chords[SHORTCUT_REMOVE], shortcut::FOCUSED)) remove_unused();
      if (imgui::shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
      if (imgui::shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();
      if (imgui::shortcut(manager.chords[SHORTCUT_MERGE], shortcut::FOCUSED) && selection.size() > 1) merge_open();
    }
    ImGui::End();

    mergePopup.trigger();
    if (ImGui::BeginPopupModal(mergePopup.label(), &mergePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      settings.mergeSpritesheetsRegionOrigin =
          glm::clamp(settings.mergeSpritesheetsRegionOrigin, (int)origin::TOP_LEFT, (int)origin::ORIGIN_CENTER);

      auto close = [&]()
      {
        mergeSelection.clear();
        mergePopup.close();
      };

      auto optionsSize = child_size_get(5);
      if (ImGui::BeginChild("##Merge Spritesheets Options", optionsSize, ImGuiChildFlags_Borders))
      {
        ImGui::SeparatorText(localize.get(LABEL_REGION_PROPERTIES_ORIGIN));
        ImGui::RadioButton(localize.get(LABEL_MERGE_SPRITESHEETS_APPEND_BOTTOM), &settings.mergeSpritesheetsOrigin,
                           anm2::APPEND_BOTTOM);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_SPRITESHEETS_BOTTOM_LEFT));
        ImGui::SameLine();
        ImGui::RadioButton(localize.get(LABEL_MERGE_SPRITESHEETS_APPEND_RIGHT), &settings.mergeSpritesheetsOrigin,
                           anm2::APPEND_RIGHT);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_SPRITESHEETS_TOP_RIGHT));

        ImGui::SeparatorText(localize.get(LABEL_OPTIONS));
        ImGui::Checkbox(localize.get(LABEL_MERGE_MAKE_SPRITESHEET_REGIONS), &settings.mergeSpritesheetsIsMakeRegions);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_MAKE_SPRITESHEET_REGIONS));

        const char* regionOriginOptions[] = {localize.get(LABEL_REGION_ORIGIN_TOP_LEFT),
                                             localize.get(LABEL_REGION_ORIGIN_CENTER)};
        ImGui::BeginDisabled(!settings.mergeSpritesheetsIsMakeRegions);
        ImGui::Combo(localize.get(LABEL_REGION_PROPERTIES_ORIGIN), &settings.mergeSpritesheetsRegionOrigin,
                     regionOriginOptions, IM_ARRAYSIZE(regionOriginOptions));
        ImGui::EndDisabled();
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);
      shortcut(manager.chords[SHORTCUT_CONFIRM]);
      ImGui::BeginDisabled(mergeSelection.size() <= 1);
      if (ImGui::Button(localize.get(BASIC_MERGE), widgetSize))
      {
        merge();
        close();
      }
      ImGui::EndDisabled();

      ImGui::SameLine();
      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) close();

      ImGui::EndPopup();
    }
    mergePopup.end();

  }
}
