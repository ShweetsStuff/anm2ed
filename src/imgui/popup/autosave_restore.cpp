#include "autosave_restore.hpp"

#include <format>

#include "path.hpp"
#include "util/imgui/layout.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::util;

namespace anm2ed::imgui
{
  void AutosaveRestore::update(Manager& manager)
  {
    if (!isInitialized)
    {
      isInitialized = true;
      isRestoreAvailable = !manager.autosaveFiles.empty();
      if (isRestoreAvailable) popup.open();
    }

    popup.trigger();

    if (isRestoreAvailable && ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize))
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
        isRestoreAvailable = false;
        popup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_NO), widgetSize))
      {
        manager.autosave_files_clear(true);
        isRestoreAvailable = false;
        popup.close();
      }

      ImGui::EndPopup();
    }
  }
}
