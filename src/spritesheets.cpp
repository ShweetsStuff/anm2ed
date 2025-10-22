#include "spritesheets.h"

#include <ranges>

#include "imgui.h"
#include "toast.h"
#include "types.h"

using namespace anm2ed::anm2;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::dialog;
using namespace anm2ed::document_manager;
using namespace anm2ed::types;
using namespace anm2ed::toast;
using namespace glm;

namespace anm2ed::spritesheets
{
  void Spritesheets::update(DocumentManager& manager, Settings& settings, Resources& resources, Dialog& dialog)
  {
    if (ImGui::Begin("Spritesheets", &settings.windowIsSpritesheets))
    {
      auto& document = *manager.get();
      auto& anm2 = document.anm2;
      auto& selection = document.selectedSpritesheets;
      auto style = ImGui::GetStyle();
      static ImGuiSelectionExternalStorage storage{};
      storage.UserData = &selection;
      storage.AdapterSetItemSelected = imgui::external_storage_set;

      auto childSize = imgui::size_with_footer_get(2);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

      if (ImGui::BeginChild("##Spritesheets Child", childSize, ImGuiChildFlags_Borders))
      {
        auto spritesheetChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 4);

        ImGuiMultiSelectIO* io = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_ClearOnEscape, selection.size(),
                                                         anm2.content.spritesheets.size());
        storage.ApplyRequests(io);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

        for (auto& [id, spritesheet] : anm2.content.spritesheets)
        {
          ImGui::PushID(id);

          if (ImGui::BeginChild("##Spritesheet Child", spritesheetChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = selection.contains(id);
            auto isReferenced = id == document.referenceSpritesheet;
            auto cursorPos = ImGui::GetCursorPos();
            auto& texture = spritesheet.texture;

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (ImGui::Selectable("##Spritesheet Selectable", isSelected, 0, spritesheetChildSize))
              document.referenceSpritesheet = id;

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

              auto viewport = ImGui::GetMainViewport();

              auto size = texture.size.x * texture.size.y > (viewport->Size.x * viewport->Size.y) * 0.5f
                              ? to_vec2(viewport->Size) * 0.5f
                              : vec2(texture.size);

              auto aspectRatio = (float)texture.size.x / texture.size.y;

              if (size.x / size.y > aspectRatio)
                size.x = size.y * aspectRatio;
              else
                size.y = size.x / aspectRatio;

              if (ImGui::BeginChild("##Spritesheet Tooltip Image Child", to_imvec2(size), ImGuiChildFlags_Borders))
                ImGui::Image(texture.id, ImGui::GetContentRegionAvail());
              ImGui::EndChild();

              ImGui::PopStyleVar();

              ImGui::SameLine();

              if (ImGui::BeginChild(
                      "##Spritesheet Info Tooltip Child",
                      ImVec2(ImGui::CalcTextSize(spritesheet.path.c_str()).x + ImGui::GetTextLineHeightWithSpacing(),
                             0)))
              {
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(spritesheet.path.c_str());
                ImGui::PopFont();
                ImGui::TextUnformatted(std::format("ID: {}", id).c_str());
                ImGui::TextUnformatted(std::format("Size: {} x {}", texture.size.x, texture.size.y).c_str());
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
            ImGui::TextUnformatted(std::format("#{} {}", id, spritesheet.path.string()).c_str());
            if (isReferenced) ImGui::PopFont();
          }
          ImGui::EndChild();

          ImGui::PopID();
        }

        io = ImGui::EndMultiSelect();
        storage.ApplyRequests(io);

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();

      ImGui::PopStyleVar();

      auto rowOneWidgetSize = imgui::widget_size_with_row_get(4);

      imgui::shortcut(settings.shortcutAdd, true);
      if (ImGui::Button("Add", rowOneWidgetSize)) dialog.spritesheet_open();
      imgui::set_item_tooltip_shortcut("Add a new spritesheet.", settings.shortcutAdd);

      if (dialog.is_selected_file(dialog::SPRITESHEET_OPEN))
      {
        manager.spritesheet_add(dialog.path);
        dialog.reset();
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      {
        if (ImGui::Button("Reload", rowOneWidgetSize))
        {
          for (auto& id : selection)
          {
            Spritesheet& spritesheet = anm2.content.spritesheets[id];
            spritesheet.reload(document.directory_get());
            toasts.add(std::format("Reloaded spritesheet #{}: {}", id, spritesheet.path.string()));
          }
        }
        ImGui::SetItemTooltip("Reloads the selected spritesheets.");
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.size() != 1);
      {
        if (ImGui::Button("Replace", rowOneWidgetSize)) dialog.spritesheet_replace();
        ImGui::SetItemTooltip("Replace the selected spritesheet with a new one.");
      }
      ImGui::EndDisabled();

      if (dialog.is_selected_file(dialog::SPRITESHEET_REPLACE))
      {
        auto& id = *selection.begin();
        Spritesheet& spritesheet = anm2.content.spritesheets[id];
        spritesheet = Spritesheet(document.directory_get(), dialog.path);
        toasts.add(std::format("Replaced spritesheet #{}: {}", id, spritesheet.path.string()));
        dialog.reset();
      }

      ImGui::SameLine();

      auto unused = anm2.spritesheets_unused();

      ImGui::BeginDisabled(unused.empty());
      {
        imgui::shortcut(settings.shortcutRemove, true);
        if (ImGui::Button("Remove Unused", rowOneWidgetSize))
        {
          for (auto& id : unused)
          {
            Spritesheet& spritesheet = anm2.content.spritesheets[id];
            toasts.add(std::format("Removed spritesheet #{}: {}", id, spritesheet.path.string()));
            anm2.spritesheet_remove(id);
          }
        }
        imgui::set_item_tooltip_shortcut("Remove all unused spritesheets (i.e., not used in any layer.).",
                                         settings.shortcutRemove);
      }
      ImGui::EndDisabled();

      auto rowTwoWidgetSize = imgui::widget_size_with_row_get(3);

      imgui::shortcut(settings.shortcutSelectAll, true);
      ImGui::BeginDisabled(selection.size() == anm2.content.spritesheets.size());
      {
        if (ImGui::Button("Select All", rowTwoWidgetSize))
          for (auto& id : anm2.content.spritesheets | std::views::keys)
            selection.insert(id);
      }
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Select all spritesheets.", settings.shortcutSelectAll);

      ImGui::SameLine();

      imgui::shortcut(settings.shortcutSelectNone, true);
      ImGui::BeginDisabled(selection.empty());
      {
        if (ImGui::Button("Select None", rowTwoWidgetSize)) selection.clear();
      }
      ImGui::EndDisabled();
      imgui::set_item_tooltip_shortcut("Unselect all spritesheets.", settings.shortcutSelectNone);

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      {
        if (ImGui::Button("Save", rowTwoWidgetSize))
        {
          for (auto& id : selection)
          {
            if (Spritesheet& spritesheet = anm2.content.spritesheets[id]; spritesheet.save(document.directory_get()))
              toasts.add(std::format("Saved spritesheet #{}: {}", id, spritesheet.path.string()));
            else
              toasts.add(std::format("Unable to save spritesheet #{}: {}", id, spritesheet.path.string()));
          }
        }
      }
      ImGui::EndDisabled();
      ImGui::SetItemTooltip("Save the selected spritesheets.");
    }
    ImGui::End();
  }
}
