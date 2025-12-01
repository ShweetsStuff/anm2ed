#include "spritesheets.h"

#include <ranges>

#include <format>

#include "log.h"
#include "document.h"
#include "filesystem_.h"
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
    auto& unused = document.spritesheet.unused;
    auto& hovered = document.spritesheet.hovered;
    auto& reference = document.spritesheet.reference;

    hovered = -1;

    if (ImGui::Begin(localize.get(LABEL_SPRITESHEETS_WINDOW), &settings.windowIsSpritesheets))
    {
      auto style = ImGui::GetStyle();

      auto context_menu = [&]()
      {
        auto copy = [&]()
        {
          if (!selection.empty())
          {
            std::string clipboardText{};
            for (auto& id : selection)
              clipboardText += anm2.content.spritesheets[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.spritesheets[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          std::string errorString{};
          document.snapshot(localize.get(EDIT_PASTE_SPRITESHEETS));
        if (anm2.spritesheets_deserialize(clipboard.get(), document.directory_get().string(), type, &errorString))
          document.change(Document::SPRITESHEETS);
        else
        {
          toasts.push(std::vformat(localize.get(TOAST_DESERIALIZE_SPRITESHEETS_FAILED),
                                   std::make_format_args(errorString)));
          logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_SPRITESHEETS_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(errorString)));
        }
        };

        if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste(merge::APPEND);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::MenuItem(localize.get(BASIC_CUT), settings.shortcutCut.c_str(), false, true);

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
        ImGui::PopStyleVar(2);
      };

      auto childSize = size_without_footer_get(2);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
      if (ImGui::BeginChild("##Spritesheets Child", childSize, ImGuiChildFlags_Borders))
      {
        auto spritesheetChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 4);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

        selection.start(anm2.content.spritesheets.size(), ImGuiMultiSelectFlags_ClearOnEscape);

        for (auto& [id, spritesheet] : anm2.content.spritesheets)
        {
          ImGui::PushID(id);

          if (ImGui::BeginChild("##Spritesheet Child", spritesheetChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = selection.contains(id);
            auto isReferenced = id == reference;
            auto cursorPos = ImGui::GetCursorPos();
            bool isTextureValid = spritesheet.texture.is_valid();
            auto& texture = isTextureValid ? spritesheet.texture : resources.icons[icon::NONE];
            auto textureRef = ImTextureRef(texture.id);
            auto tintColor = !isTextureValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            auto pathString = spritesheet.path.empty() ? std::string{anm2::NO_PATH} : spritesheet.path.string();
            auto pathCStr = pathString.c_str();

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (ImGui::Selectable("##Spritesheet Selectable", isSelected, 0, spritesheetChildSize)) reference = id;
            if (ImGui::IsItemHovered())
            {
              if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
              {
                filesystem::WorkingDirectory workingDirectory(document.directory_get().string());
                dialog.file_explorer_open(spritesheet.path.parent_path().string());
              }
              hovered = id;
            }
            if (newSpritesheetId == id)
            {
              ImGui::SetScrollHereY(0.5f);
              newSpritesheetId = -1;
            }

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
                ImGui::ImageWithBg(textureRef, to_imvec2(textureSize), ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);
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

                if (!isTextureValid)
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
            ImGui::ImageWithBg(textureRef, imageSize, ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);

            ImGui::SetCursorPos(
                ImVec2(spritesheetChildSize.y + style.ItemSpacing.x,
                       spritesheetChildSize.y - spritesheetChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));

            if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(id, pathCStr)).c_str());
            if (isReferenced) ImGui::PopFont();

            context_menu();
          }

          ImGui::EndChild();

          ImGui::PopID();
        }

        ImGui::PopStyleVar();
        context_menu();
        selection.finish();
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();

      auto rowOneWidgetSize = widget_size_with_row_get(3);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), rowOneWidgetSize)) dialog.file_open(dialog::SPRITESHEET_OPEN);
      set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_SPRITESHEET), settings.shortcutAdd);

      if (dialog.is_selected(dialog::SPRITESHEET_OPEN))
      {
        document.spritesheet_add(dialog.path);
        newSpritesheetId = document.spritesheet.reference;
        dialog.reset();
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      {
        if (ImGui::Button(localize.get(BASIC_RELOAD), rowOneWidgetSize))
        {
          auto reload = [&]()
          {
            for (auto& id : selection)
            {
              anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
              spritesheet.reload(document.directory_get());
              auto pathString = spritesheet.path.string();
              toasts.push(std::vformat(localize.get(TOAST_RELOAD_SPRITESHEET),
                                       std::make_format_args(id, pathString)));
              logger.info(std::vformat(localize.get(TOAST_RELOAD_SPRITESHEET, anm2ed::ENGLISH),
                                       std::make_format_args(id, pathString)));
            }
          };

          DOCUMENT_EDIT(document, localize.get(EDIT_RELOAD_SPRITESHEETS), Document::SPRITESHEETS, reload());
        }
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RELOAD_SPRITESHEETS));
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.size() != 1);
      {
        if (ImGui::Button(localize.get(BASIC_REPLACE), rowOneWidgetSize)) dialog.file_open(dialog::SPRITESHEET_REPLACE);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REPLACE_SPRITESHEET));
      }
      ImGui::EndDisabled();

      if (dialog.is_selected(dialog::SPRITESHEET_REPLACE))
      {
        auto replace = [&]()
        {
          auto& id = *selection.begin();
          anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
          spritesheet = anm2::Spritesheet(document.directory_get().string(), dialog.path);
          auto pathString = spritesheet.path.string();
          toasts.push(std::vformat(localize.get(TOAST_REPLACE_SPRITESHEET), std::make_format_args(id, pathString)));
          logger.info(std::vformat(localize.get(TOAST_REPLACE_SPRITESHEET, anm2ed::ENGLISH),
                                   std::make_format_args(id, pathString)));
        };

        DOCUMENT_EDIT(document, localize.get(EDIT_REPLACE_SPRITESHEET), Document::SPRITESHEETS, replace());
        dialog.reset();
      }

      auto rowTwoWidgetSize = widget_size_with_row_get(2);

      ImGui::BeginDisabled(unused.empty());
      {
        shortcut(manager.chords[SHORTCUT_REMOVE]);
          if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), rowTwoWidgetSize))
        {
          auto remove_unused = [&]()
          {
            for (auto& id : unused)
            {
              anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
              auto pathString = spritesheet.path.string();
              toasts.push(std::vformat(localize.get(TOAST_REMOVE_SPRITESHEET),
                                       std::make_format_args(id, pathString)));
              logger.info(std::vformat(localize.get(TOAST_REMOVE_SPRITESHEET, anm2ed::ENGLISH),
                                       std::make_format_args(id, pathString)));
              anm2.content.spritesheets.erase(id);
            }
            unused.clear();
          };

          DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_SPRITESHEETS), Document::SPRITESHEETS,
                        remove_unused());
        }
        set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_SPRITESHEETS), settings.shortcutRemove);
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      {
        if (ImGui::Button(localize.get(BASIC_SAVE), rowTwoWidgetSize))
        {
          for (auto& id : selection)
          {
            anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
            auto pathString = spritesheet.path.string();
            if (spritesheet.save(document.directory_get().string()))
            {
              toasts.push(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET),
                                       std::make_format_args(id, pathString)));
              logger.info(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET, anm2ed::ENGLISH),
                                       std::make_format_args(id, pathString)));
            }
            else
            {
              toasts.push(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED),
                                       std::make_format_args(id, pathString)));
              logger.error(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED, anm2ed::ENGLISH),
                                        std::make_format_args(id, pathString)));
            }
          }
        }
      }
      ImGui::EndDisabled();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SAVE_SPRITESHEETS));
    }
    ImGui::End();
  }
}
