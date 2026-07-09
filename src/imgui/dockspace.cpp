#include "dockspace.hpp"

namespace anm2ed::imgui
{
  bool Dockspace::is_canvas_focused_get() const { return isCanvasFocused; }

  void Dockspace::tick(Manager& manager, Settings& settings, float deltaSeconds)
  {
    if (auto document = manager.get(); document)
      if (settings.windowIsAnimationPreview) animationPreview.tick(manager, settings, deltaSeconds);
  }

  void Dockspace::update(Taskbar& taskbar, Documents& documents, Manager& manager, Settings& settings,
                         Resources& resources, Dialog& dialog, Clipboard& clipboard)
  {
    isCanvasFocused = false;

    auto viewport = ImGui::GetMainViewport();
    auto windowHeight = viewport->Size.y - taskbar.height - documents.height;
    if (windowHeight < 1.0f) windowHeight = 1.0f;

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
          if (settings.windowIsAnimations) window_update(animations, manager, settings, resources, dialog, clipboard);
          if (settings.windowIsRegions) window_update(regions, manager, settings, resources, dialog, clipboard);
          if (settings.windowIsEvents) window_update(events, manager, settings, resources, dialog, clipboard);
          if (settings.windowIsFrameProperties) frameProperties.update(manager, settings);
          if (settings.windowIsLayers) window_update(layers, manager, settings, resources, dialog, clipboard);
          if (settings.windowIsNulls) window_update(nulls, manager, settings, resources, dialog, clipboard);
          if (settings.windowIsOnionskin) onionskin.update(manager, settings);
          if (settings.windowIsOverlays) window_update(overlays, manager, settings, resources, dialog, clipboard);
          if (settings.windowIsShaders) shaders.update(manager, settings, resources, dialog);
          if (settings.windowIsSounds) window_update(sounds, manager, settings, resources, dialog, clipboard);
          if (settings.windowIsSpritesheetEditor) spritesheetEditor.update(manager, settings, resources);
          if (settings.windowIsSpritesheets)
            window_update(spritesheets, manager, settings, resources, dialog, clipboard);
          if (settings.windowIsTimeline) timeline.update(manager, settings, resources, clipboard);
          if (settings.windowIsTools) tools.update(manager, settings, resources);
          isCanvasFocused = (settings.windowIsAnimationPreview && animationPreview.is_focused_get()) ||
                            (settings.windowIsSpritesheetEditor && spritesheetEditor.is_focused_get());
        }
      }
      else
        welcome.update(manager, resources, dialog, taskbar, documents);

      autosaveRestore.update(manager);
    }
    ImGui::End();
  }
}
