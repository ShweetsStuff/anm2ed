#include "dockspace.hpp"

#include <imgui/imgui.h>

namespace anm2ed::ui_new
{
  Dockspace::Dockspace()
  {
    window_register("Animations",
                    Window::ELEMENT,
                    BUTTON_ADD | BUTTON_DUPLICATE | BUTTON_MERGE | BUTTON_REMOVE | BUTTON_DEFAULT,
                    CONTEXT_MENU_NONE,
                    anm2_new::Anm2::Element::ANIMATION);

    auto sharedElementButtons = BUTTON_ADD | BUTTON_REMOVE_UNUSED;
    auto sharedElementContextMenu =
        CONTEXT_MENU_UNDO | CONTEXT_MENU_REDO | CONTEXT_MENU_ADD | CONTEXT_MENU_COPY | CONTEXT_MENU_PASTE;

    window_register(
        "Layers", Window::ELEMENT, sharedElementButtons, sharedElementContextMenu, anm2_new::Anm2::Element::LAYER);
    window_register(
        "Events", Window::ELEMENT, sharedElementButtons, sharedElementContextMenu, anm2_new::Anm2::Element::EVENT);
    window_register(
        "Nulls", Window::ELEMENT, sharedElementButtons, sharedElementContextMenu, anm2_new::Anm2::Element::NULL_ELEMENT);
  }

  void Dockspace::window_register(const char* label, Window::Type windowType, ButtonOptions buttonOptions,
                                  ContextMenuOptions contextMenuOptions, anm2_new::Anm2::Element::Type elementType)
  {
    windows.emplace_back(label, windowType, buttonOptions, contextMenuOptions, elementType);
  }

  void Dockspace::update(Taskbar& taskbar, Documents& documents, ManagerNew& manager)
  {
    auto* viewport = ImGui::GetMainViewport();
    auto* document = manager.get();
    auto topOffset = taskbar.height + documents.height;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + topOffset));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - topOffset));
    ImGui::SetNextWindowViewport(viewport->ID);

    auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                 ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin("##DockspaceNew", nullptr, flags))
    {
      auto dockspaceId = ImGui::GetID("##DockspaceNewID");
      ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f));
    }

    ImGui::End();
    ImGui::PopStyleVar(3);

    for (auto& window : windows)
      window.update(document);
  }
}
