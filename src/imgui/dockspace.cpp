#include "dockspace.h"

namespace anm2ed::imgui
{
  void Dockspace::tick(Manager& manager, Settings& settings)
  {
    if (auto document = manager.get(); document)
      if (settings.windowIsAnimationPreview) animationPreview.tick(manager, *document, settings);
  }

  void Dockspace::update(Taskbar& taskbar, Documents& documents, Manager& manager, Settings& settings,
                         Resources& resources, Dialog& dialog, Clipboard& clipboard)
  {

    auto viewport = ImGui::GetMainViewport();
    auto windowHeight = viewport->Size.y - taskbar.height - documents.height;

    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + taskbar.height + documents.height));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, windowHeight));

    if (ImGui::Begin("##DockSpace", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoNavFocus))
    {
      if (auto document = manager.get(); document)
      {
        if (ImGui::DockSpace(ImGui::GetID("##DockSpace"), ImVec2(), ImGuiDockNodeFlags_PassthruCentralNode))
        {
          if (settings.windowIsAnimationPreview) animationPreview.update(manager, settings, resources);
          if (settings.windowIsAnimations) animations.update(manager, settings, resources, clipboard);
          if (settings.windowIsEvents) events.update(manager, settings, resources, clipboard);
          if (settings.windowIsFrameProperties) frameProperties.update(manager, settings);
          if (settings.windowIsLayers) layers.update(manager, settings, resources, clipboard);
          if (settings.windowIsNulls) nulls.update(manager, settings, resources, clipboard);
          if (settings.windowIsOnionskin) onionskin.update(settings);
          if (settings.windowIsSounds) sounds.update(manager, settings, resources, dialog, clipboard);
          if (settings.windowIsSpritesheetEditor) spritesheetEditor.update(manager, settings, resources);
          if (settings.windowIsSpritesheets) spritesheets.update(manager, settings, resources, dialog, clipboard);
          if (settings.windowIsTimeline) timeline.update(manager, settings, resources, clipboard);
          if (settings.windowIsTools) tools.update(manager, settings, resources);
        }
      }
      else
        welcome.update(manager, resources, dialog, taskbar, documents);
    }
    ImGui::End();
  }
}
