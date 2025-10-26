#include "spritesheets.h"

#include <ranges>

#include "imgui.h"
#include "toast.h"

using namespace anm2ed::anm2;
using namespace anm2ed::clipboard;
using namespace anm2ed::manager;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::dialog;
using namespace anm2ed::document;
using namespace anm2ed::types;
using namespace anm2ed::toast;
using namespace glm;

namespace anm2ed::spritesheets
{
  void Spritesheets::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog,
                            Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& multiSelect = document.spritesheetMultiSelect;
    auto& unused = document.unusedSpritesheetIDs;
    auto& hovered = document.hoveredSpritesheet;
    auto& reference = document.referenceSpritesheet;

    if (ImGui::Begin("Spritesheets", &settings.windowIsSpritesheets))
    {
      auto style = ImGui::GetStyle();

      auto context_menu = [&]()
      {
        auto copy = [&]()
        {
          if (!multiSelect.empty())
          {
            std::string clipboardText{};
            for (auto& id : multiSelect)
              clipboardText += anm2.content.spritesheets[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.spritesheets[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          auto clipboardText = clipboard.get();
          document.spritesheets_deserialize(clipboardText, type);
        };

        if (imgui::shortcut(settings.shortcutCopy, shortcut::FOCUSED)) copy();
        if (imgui::shortcut(settings.shortcutPaste, shortcut::FOCUSED)) paste(merge::APPEND);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
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
        ImGui::PopStyleVar(2);
      };

      auto childSize = imgui::size_without_footer_get(2);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

      if (ImGui::BeginChild("##Spritesheets Child", childSize, ImGuiChildFlags_Borders))
      {
        auto spritesheetChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 4);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

        multiSelect.start(anm2.content.spritesheets.size());

        for (auto& [id, spritesheet] : anm2.content.spritesheets)
        {
          ImGui::PushID(id);

          if (ImGui::BeginChild("##Spritesheet Child", spritesheetChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = multiSelect.contains(id);
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
            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

              auto viewport = ImGui::GetMainViewport();

              auto textureSize = texture.size.x * texture.size.y > (viewport->Size.x * viewport->Size.y) * 0.5f
                                     ? to_vec2(viewport->Size) * 0.5f
                                     : vec2(texture.size);
              auto aspectRatio = (float)texture.size.x / texture.size.y;

              if (textureSize.x / textureSize.y > aspectRatio)
                textureSize.x = textureSize.y * aspectRatio;
              else
                textureSize.y = textureSize.x / aspectRatio;

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
            auto aspectRatio = (float)texture.size.x / texture.size.y;

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
            ImGui::Text(SPRITESHEET_FORMAT, id, path);
            if (isReferenced) ImGui::PopFont();

            context_menu();
          }
          ImGui::EndChild();

          ImGui::PopID();
        }

        multiSelect.finish();

        ImGui::PopStyleVar(2);

        context_menu();
      }
      ImGui::EndChild();

      auto rowOneWidgetSize = imgui::widget_size_with_row_get(4);

      imgui::shortcut(settings.shortcutAdd);
      if (ImGui::Button("Add", rowOneWidgetSize)) dialog.spritesheet_open();
      imgui::set_item_tooltip_shortcut("Add a new spritesheet.", settings.shortcutAdd);

      if (dialog.is_selected_file(dialog::SPRITESHEET_OPEN))
      {
        document.spritesheet_add(dialog.path);
        dialog.reset();
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(multiSelect.empty());
      {
        if (ImGui::Button("Reload", rowOneWidgetSize))
        {
          for (auto& id : multiSelect)
          {
            Spritesheet& spritesheet = anm2.content.spritesheets[id];
            spritesheet.reload(document.directory_get());
            toasts.info(std::format("Reloaded spritesheet #{}: {}", id, spritesheet.path.string()));
          }
        }
        ImGui::SetItemTooltip("Reloads the selected spritesheets.");
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(multiSelect.size() != 1);
      {
        if (ImGui::Button("Replace", rowOneWidgetSize)) dialog.spritesheet_replace();
        ImGui::SetItemTooltip("Replace the selected spritesheet with a new one.");
      }
      ImGui::EndDisabled();

      if (dialog.is_selected_file(dialog::SPRITESHEET_REPLACE))
      {
        auto& id = *multiSelect.begin();
        Spritesheet& spritesheet = anm2.content.spritesheets[id];
        spritesheet = Spritesheet(document.directory_get(), dialog.path);
        toasts.info(std::format("Replaced spritesheet #{}: {}", id, spritesheet.path.string()));
        dialog.reset();
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(unused.empty());
      {
        imgui::shortcut(settings.shortcutRemove);
        if (ImGui::Button("Remove Unused", rowOneWidgetSize))
        {
          for (auto& id : unused)
          {
            Spritesheet& spritesheet = anm2.content.spritesheets[id];
            toasts.info(std::format("Removed spritesheet #{}: {}", id, spritesheet.path.string()));
            anm2.spritesheet_remove(id);
          }
          unused.clear();
          document.change(change::SPRITESHEETS);
        }
        imgui::set_item_tooltip_shortcut("Remove all unused spritesheets (i.e., not used in any layer.).",
                                         settings.shortcutRemove);
      }
      ImGui::EndDisabled();

      auto rowTwoWidgetSize = imgui::widget_size_with_row_get(3);

      imgui::shortcut(settings.shortcutSelectAll);
      ImGui::BeginDisabled(multiSelect.size() == anm2.content.spritesheets.size());
      {
        if (ImGui::Button("Select All", rowTwoWidgetSize))
          for (auto& id : anm2.content.spritesheets | std::views::keys)
            multiSelect.insert(id);
      }
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Select all spritesheets.", settings.shortcutSelectAll);

      ImGui::SameLine();

      imgui::shortcut(settings.shortcutSelectNone);
      ImGui::BeginDisabled(multiSelect.empty());
      if (ImGui::Button("Select None", rowTwoWidgetSize)) multiSelect.clear();
      imgui::set_item_tooltip_shortcut("Unselect all spritesheets.", settings.shortcutSelectNone);
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(multiSelect.empty());
      {
        if (ImGui::Button("Save", rowTwoWidgetSize))
        {
          for (auto& id : multiSelect)
          {
            Spritesheet& spritesheet = anm2.content.spritesheets[id];
            if (spritesheet.save(document.directory_get()))
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