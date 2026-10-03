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
          auto panel_make = [&](PanelState& state)
          { return Panel{manager, settings, resources, dialog, clipboard, *document, state}; };
          auto panel_update = [&](PanelState& state, void (*update)(Panel&))
          {
            auto panel = panel_make(state);
            update(panel);
          };
          if (settings.windowIsAnimationPreview) animationPreview.update(manager, settings, resources);
          if (settings.windowIsAnimations)
          {
            auto panel = panel_make(animations.state);
            animations_update(panel, animations);
          }
          if (settings.windowIsRegions)
          {
            auto panel = panel_make(regions.state);
            regions_update(panel, regions);
          }
          if (settings.windowIsEvents) panel_update(events, events_update);
          if (settings.windowIsFrameProperties) frame_properties_update(manager, settings, frameProperties);
          if (settings.windowIsLayers) panel_update(layers, layers_update);
          if (settings.windowIsNulls) panel_update(nulls, nulls_update);
          if (settings.windowIsOnionskin) onionskin_update(manager, settings);
          if (settings.windowIsShaders)
          {
            auto panel = panel_make(shaders.state);
            shaders_update(panel, shaders);
          }
          if (settings.windowIsSounds) panel_update(sounds, sounds_update);
          if (settings.windowIsSpritesheetEditor) spritesheetEditor.update(manager, settings, resources);
          if (settings.windowIsSpritesheets)
          {
            auto panel = panel_make(spritesheets.state);
            spritesheets_update(panel, spritesheets);
          }
          if (settings.windowIsTimeline) timeline.update(manager, settings, resources, clipboard);
          if (settings.windowIsTools) tools_update(manager, settings, resources, toolsColorEditPopup);
          isCanvasFocused = (settings.windowIsAnimationPreview && animationPreview.is_focused_get()) ||
                            (settings.windowIsSpritesheetEditor && spritesheetEditor.is_focused_get());
        }
      }
      else
        welcome_update(manager, resources, dialog, taskbar, documents);

      autosaveRestore.update(manager);
    }
    ImGui::End();
  }
}
