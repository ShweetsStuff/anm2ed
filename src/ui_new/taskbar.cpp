#include "taskbar.hpp"

#include <imgui/imgui.h>

namespace anm2ed::ui_new
{
  void Taskbar::update(Dialog& dialog, ManagerNew& manager, bool& isQuitting)
  {
    if (!ImGui::BeginMainMenuBar()) return;

    auto* document = manager.get();

    height = ImGui::GetWindowSize().y;

    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Open")) dialog.file_open(Dialog::ANM2_OPEN);

      if (ImGui::MenuItem("Save", nullptr, false, document && document->isOpen))
      {
        if (!document->path.empty())
          document->save();
        else
          dialog.file_save(Dialog::ANM2_SAVE);
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Exit")) isQuitting = true;

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}
