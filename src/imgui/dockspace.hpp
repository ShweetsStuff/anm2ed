#pragma once

#include "documents.hpp"
#include "popup/autosave_restore.hpp"
#include "taskbar.hpp"
#include "window/animation_preview.hpp"
#include "window/panel.hpp"
#include "window/shaders.hpp"
#include "window/spritesheet_editor.hpp"
#include "window/timeline/timeline.hpp"

namespace anm2ed::imgui
{
  class Dockspace
  {
    AnimationPreview animationPreview;
    bool isCanvasFocused{};
    AnimationsPanel animations{};
    RegionsPanel regions{};
    PanelState events{};
    FramePropertiesPanel frameProperties{};
    PanelState layers{};
    PanelState nulls{};
    ShadersPanel shaders{};
    SpritesheetEditor spritesheetEditor;
    SpritesheetsPanel spritesheets{};
    PanelState sounds{};
    Timeline timeline;
    PopupHelper toolsColorEditPopup{PopupHelper(LABEL_TOOLS_COLOR_EDIT_POPUP, POPUP_TO_CONTENT, POPUP_BY_ITEM)};
    AutosaveRestore autosaveRestore;

  public:
    bool is_canvas_focused_get() const;
    void tick(Manager&, Settings&, float);
    void update(Taskbar&, Documents&, Manager&, Settings&, Resources&, Dialog&, Clipboard&);
  };
}
