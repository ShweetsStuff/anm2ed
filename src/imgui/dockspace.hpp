#pragma once

#include "popup/autosave_restore.hpp"
#include "documents.hpp"
#include "taskbar.hpp"
#include "window/animation_preview.hpp"
#include "window/frame_properties.hpp"
#include "window/onionskin.hpp"
#include "window/spritesheet_editor.hpp"
#include "window/timeline.hpp"
#include "window/tools.hpp"
#include "window/welcome.hpp"
#include "window/window.hpp"

namespace anm2ed::imgui
{
  class Dockspace
  {
    AnimationPreview animationPreview;
    bool isCanvasFocused{};
    Window animations{animations_window_register()};
    Window regions{regions_window_register()};
    Window events{events_window_register()};
    FrameProperties frameProperties;
    Window layers{layers_window_register()};
    Window nulls{nulls_window_register()};
    Onionskin onionskin;
    SpritesheetEditor spritesheetEditor;
    Window spritesheets{spritesheets_window_register()};
    Window sounds{sounds_window_register()};
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
