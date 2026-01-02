#include "welcome.h"

#include <ranges>

#include "path_.h"
#include "strings.h"

using namespace anm2ed::util;
using namespace anm2ed::resource;

namespace anm2ed::imgui
{
  void Welcome::update(Manager& manager, Resources& resources, Dialog& dialog, Taskbar& taskbar, Documents& documents)
  {
    auto viewport = ImGui::GetMainViewport();
    auto windowHeight = viewport->Size.y - taskbar.height - documents.height;
    if (windowHeight < 1.0f)
      windowHeight = 1.0f;

    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + taskbar.height + documents.height));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, windowHeight));

    if (ImGui::Begin(localize.get(LABEL_WELCOME_WINDOW), nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse))
    {

      ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);
      ImGui::TextUnformatted(localize.get(LABEL_APPLICATION_NAME));
      ImGui::PopFont();

      ImGui::TextUnformatted(localize.get(LABEL_WELCOME_DESCRIPTION));

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(localize.get(BASIC_NEW), widgetSize))
        dialog.file_save(Dialog::ANM2_NEW); // handled in taskbar.cpp
      ImGui::SameLine();
      if (ImGui::Button(localize.get(BASIC_OPEN), widgetSize))
        dialog.file_open(Dialog::ANM2_OPEN); // handled in taskbar.cpp

      if (ImGui::BeginChild("##Recent Files Child", {}, ImGuiChildFlags_Borders))
      {
        auto recentFiles = manager.recent_files_ordered();
        for (auto [i, file] : std::views::enumerate(recentFiles))
        {
          ImGui::PushID(i);

          auto label = std::format(FILE_LABEL_FORMAT, path::to_utf8(file.filename()), path::to_utf8(file));

          if (ImGui::Selectable(label.c_str()))
          {
            manager.open(file);
            ImGui::PopID();
            break;
          }

          ImGui::PopID();
        }
      }
      ImGui::EndChild();
    }
    ImGui::End();

    if (!manager.autosaveFiles.empty() && !restorePopup.is_open()) restorePopup.open();

    restorePopup.trigger();

    if (ImGui::BeginPopupModal(restorePopup.label(), &restorePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      ImGui::TextUnformatted(localize.get(LABEL_RESTORE_AUTOSAVES_PROMPT));

      auto childSize = child_size_get(5);

      if (ImGui::BeginChild("##Restore Files Child", childSize, ImGuiChildFlags_Borders,
                            ImGuiWindowFlags_HorizontalScrollbar))
      {
        for (auto& file : manager.autosaveFiles)
        {
          auto label = std::format(FILE_LABEL_FORMAT, path::to_utf8(file.filename()), path::to_utf8(file));
          ImGui::TextUnformatted(label.c_str());
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(localize.get(BASIC_YES), widgetSize))
      {
        manager.autosave_files_open();
        restorePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_NO), widgetSize))
      {
        manager.autosave_files_clear();
        restorePopup.close();
      }

      ImGui::EndPopup();
    }

    selectable_input_text_id() = {};
  }

}
