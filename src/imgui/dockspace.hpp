#pragma once

#include "documents.hpp"
#include "popup/autosave_restore.hpp"
#include "taskbar.hpp"
#include "window/animation_preview.hpp"
#include "window/frame_properties.hpp"
#include "window/onionskin.hpp"
#include "window/panel.hpp"
#include "window/shaders.hpp"
#include "window/spritesheet_editor.hpp"
#include "window/timeline/timeline.hpp"
#include "window/tools.hpp"
#include "window/welcome.hpp"

namespace anm2ed::imgui
{
  class Dockspace
  {
    AnimationPreview animationPreview;
    bool isCanvasFocused{};
    AnimationsPanel animations{};
    RegionsPanel regions{};
    PanelState events{};
    FrameProperties frameProperties;
    PanelState layers{};
    PanelState nulls{};
    Onionskin onionskin;
    ShadersPanel shaders{};
    SpritesheetEditor spritesheetEditor;
    SpritesheetsPanel spritesheets{};
    PanelState sounds{};
    Timeline timeline;
    Tools tools;
    Welcome welcome;
    AutosaveRestore autosaveRestore;

  public:
    bool is_canvas_focused_get() const;
    void tick(Manager&, Settings&, float);
    void update(Taskbar&, Documents&, Manager&, Settings&, Resources&, Dialog&, Clipboard&);
  };
}
