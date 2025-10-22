#include "dockspace.h"

#include "animations.h"
#include "onionskin.h"
#include "tools.h"

using namespace anm2ed::animations;
using namespace anm2ed::dialog;
using namespace anm2ed::document_manager;
using namespace anm2ed::documents;
using namespace anm2ed::playback;
using namespace anm2ed::resources;
using namespace anm2ed::settings;
using namespace anm2ed::taskbar;

namespace anm2ed::dockspace
{
  void Dockspace::update(Taskbar& taskbar, Documents& documents, DocumentManager& manager, Settings& settings,
                         Resources& resources, Dialog& dialog, Playback& playback)
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
      if (ImGui::DockSpace(ImGui::GetID("##DockSpace"), ImVec2(), ImGuiDockNodeFlags_PassthruCentralNode))
      {
        if (manager.get())
        {
          if (settings.windowIsAnimationPreview) animationPreview.update(manager, settings, resources, playback);
          if (settings.windowIsAnimations) animations.update(manager, settings, resources);
          if (settings.windowIsEvents) events.update(manager, settings, resources);
          if (settings.windowIsFrameProperties) frameProperties.update(manager, settings);
          if (settings.windowIsLayers) layers.update(manager, settings, resources);
          if (settings.windowIsNulls) nulls.update(manager, settings, resources);
          if (settings.windowIsOnionskin) onionskin.update(settings);
          if (settings.windowIsSpritesheetEditor) spritesheetEditor.update(manager, settings, resources);
          if (settings.windowIsSpritesheets) spritesheets.update(manager, settings, resources, dialog);
          if (settings.windowIsTimeline) timeline.update(manager, settings, resources, playback);
          if (settings.windowIsTools) tools.update(settings, resources);
        }
      }
    }
    ImGui::End();
  }
}
