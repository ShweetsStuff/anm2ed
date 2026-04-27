#include "documents.hpp"

#include <imgui/imgui.h>

namespace anm2ed::ui_new
{
  void Documents::update(Taskbar& taskbar, ManagerNew& manager)
  {
    auto* viewport = ImGui::GetMainViewport();
    auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                 ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    auto styleHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
    height = manager.documents.empty() ? 0.0f : styleHeight;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + taskbar.height));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (!ImGui::Begin("##DocumentsNew", nullptr, flags))
    {
      ImGui::End();
      ImGui::PopStyleVar(2);
      return;
    }

    height = ImGui::GetWindowSize().y;

    if (ImGui::BeginTabBar("##DocumentsTabs"))
    {
      for (int i = 0; i < (int)manager.documents.size(); ++i)
      {
        auto& document = manager.documents[i];
        auto labelPath = document.filename_get();
        auto label = labelPath.empty() ? "Untitled" : labelPath.string();
        auto flags =
            (manager.selected == i && syncedSelected != manager.selected) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

        if (ImGui::BeginTabItem(label.c_str(), nullptr, flags))
        {
          if (manager.selected != i) manager.set(i);
          ImGui::EndTabItem();
        }

        if (ImGui::IsItemHovered() && !document.path.empty()) ImGui::SetTooltip("%s", document.path.string().c_str());
      }

      ImGui::EndTabBar();
    }

    syncedSelected = manager.selected;

    ImGui::End();
    ImGui::PopStyleVar(2);
  }
}
