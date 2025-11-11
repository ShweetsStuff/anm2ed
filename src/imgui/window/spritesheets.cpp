#include "spritesheets.h"

#include <ranges>

#include "document.h"
#include "toast.h"

using namespace anm2ed::types;
using namespace anm2ed::resource;
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

    if (ImGui::Begin("Spritesheets", &settings.windowIsSpritesheets))
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
          document.snapshot("Paste Spritesheet(s)");
          if (anm2.spritesheets_deserialize(clipboard.get(), document.directory_get().string(), type, &errorString))
            document.change(Document::SPRITESHEETS);
          else
            toasts.error(std::format("Failed to deserialize spritesheet(s): {}", errorString));
        };

        if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste(merge::APPEND);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::MenuItem("Cut", settings.shortcutCut.c_str(), false, true);

          if (ImGui::MenuItem("Copy", settings.shortcutCopy.c_str(), false, !selection.empty() || hovered > -1)) copy();

          if (ImGui::BeginMenu("Paste", !clipboard.is_empty()))
          {
            if (ImGui::MenuItem("Append", settings.shortcutPaste.c_str())) paste(merge::APPEND);
            if (ImGui::MenuItem("Replace")) paste(merge::REPLACE);

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
            auto& texture = spritesheet.texture.is_valid() ? spritesheet.texture : resources.icons[icon::NONE];
            auto path = spritesheet.path.empty() ? anm2::NO_PATH : spritesheet.path.c_str();

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (ImGui::Selectable("##Spritesheet Selectable", isSelected, 0, spritesheetChildSize)) reference = id;
            if (ImGui::IsItemHovered()) hovered = id;

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
            auto viewport = ImGui::GetMainViewport();
            auto textureSize = texture.size.x * texture.size.y > (viewport->Size.x * viewport->Size.y) * 0.5f
                                   ? to_vec2(viewport->Size) * 0.5f
                                   : vec2(texture.size);
            auto aspectRatio = (float)texture.size.x / texture.size.y;

            if (textureSize.x / textureSize.y > aspectRatio)
              textureSize.x = textureSize.y * aspectRatio;
            else
              textureSize.y = textureSize.x / aspectRatio;

            auto textWidth = ImGui::CalcTextSize(path).x;
            auto tooltipPadding = style.WindowPadding.x * 4.0f;
            auto minWidth = textureSize.x + style.ItemSpacing.x + textWidth + tooltipPadding;
            ImGui::SetNextWindowSize(ImVec2(minWidth, 0), ImGuiCond_Appearing);

            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

              ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
              if (ImGui::BeginChild("##Spritesheet Tooltip Image Child", to_imvec2(textureSize),
                                    ImGuiChildFlags_Borders))
                ImGui::Image(texture.id, ImGui::GetContentRegionAvail());
              ImGui::PopStyleVar();
              ImGui::EndChild();

              ImGui::PopStyleVar();

              ImGui::SameLine();

              if (ImGui::BeginChild("##Spritesheet Info Tooltip Child"))
              {
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(path);
                ImGui::PopFont();
                ImGui::Text("ID: %d", id);
                ImGui::Text("Size: %d x %d", texture.size.x, texture.size.y);
              }
              ImGui::EndChild();

              ImGui::EndTooltip();
            }
            ImGui::PopStyleVar(2);

            auto imageSize = to_imvec2(vec2(spritesheetChildSize.y));
            aspectRatio = (float)texture.size.x / texture.size.y;

            if (imageSize.x / imageSize.y > aspectRatio)
              imageSize.x = imageSize.y * aspectRatio;
            else
              imageSize.y = imageSize.x / aspectRatio;

            ImGui::SetCursorPos(cursorPos);
            ImGui::Image(texture.id, imageSize);

            ImGui::SetCursorPos(
                ImVec2(spritesheetChildSize.y + style.ItemSpacing.x,
                       spritesheetChildSize.y - spritesheetChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));

            if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
            ImGui::Text(anm2::SPRITESHEET_FORMAT_C, id, path);
            if (isReferenced) ImGui::PopFont();

            context_menu();
          }

          ImGui::EndChild();

          ImGui::PopID();
        }

        ImGui::PopStyleVar(2);

        context_menu();
        selection.finish();
      }
      ImGui::EndChild();

      auto rowOneWidgetSize = widget_size_with_row_get(3);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button("Add", rowOneWidgetSize)) dialog.file_open(dialog::SPRITESHEET_OPEN);
      set_item_tooltip_shortcut("Add a new spritesheet.", settings.shortcutAdd);

      if (dialog.is_selected(dialog::SPRITESHEET_OPEN))
      {
        document.spritesheet_add(dialog.path);
        dialog.reset();
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      {
        if (ImGui::Button("Reload", rowOneWidgetSize))
        {
          auto reload = [&]()
          {
            for (auto& id : selection)
            {
              anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
              spritesheet.reload(document.directory_get().string());
              toasts.info(std::format("Reloaded spritesheet #{}: {}", id, spritesheet.path.string()));
            }
          };

          DOCUMENT_EDIT(document, "Reload Spritesheet(s)", Document::SPRITESHEETS, reload());
        }
        ImGui::SetItemTooltip("Reloads the selected spritesheets.");
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.size() != 1);
      {
        if (ImGui::Button("Replace", rowOneWidgetSize)) dialog.file_open(dialog::SPRITESHEET_REPLACE);
        ImGui::SetItemTooltip("Replace the selected spritesheet with a new one.");
      }
      ImGui::EndDisabled();

      if (dialog.is_selected(dialog::SPRITESHEET_REPLACE))
      {
        auto replace = [&]()
        {
          auto& id = *selection.begin();
          anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
          spritesheet = anm2::Spritesheet(document.directory_get().string(), dialog.path);
          toasts.info(std::format("Replaced spritesheet #{}: {}", id, spritesheet.path.string()));
        };

        DOCUMENT_EDIT(document, "Replace Spritesheet", Document::SPRITESHEETS, replace());
        dialog.reset();
      }

      auto rowTwoWidgetSize = widget_size_with_row_get(2);

      ImGui::BeginDisabled(unused.empty());
      {
        shortcut(manager.chords[SHORTCUT_REMOVE]);
        if (ImGui::Button("Remove Unused", rowTwoWidgetSize))
        {
          auto remove_unused = [&]()
          {
            for (auto& id : unused)
            {
              anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
              toasts.info(std::format("Removed spritesheet #{}: {}", id, spritesheet.path.string()));
              anm2.content.spritesheets.erase(id);
            }
            unused.clear();
          };

          DOCUMENT_EDIT(document, "Remove Unused Spritesheets", Document::SPRITESHEETS, remove_unused());
        }
        set_item_tooltip_shortcut("Remove all unused spritesheets (i.e., not used in any layer.).",
                                  settings.shortcutRemove);
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      {
        if (ImGui::Button("Save", rowTwoWidgetSize))
        {
          for (auto& id : selection)
          {
            anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
            if (spritesheet.save(document.directory_get().string()))
              toasts.info(std::format("Saved spritesheet #{}: {}", id, spritesheet.path.string()));
            else
              toasts.info(std::format("Unable to save spritesheet #{}: {}", id, spritesheet.path.string()));
          }
        }
      }
      ImGui::EndDisabled();
      ImGui::SetItemTooltip("Save the selected spritesheets.");
    }
    ImGui::End();
  }
}
