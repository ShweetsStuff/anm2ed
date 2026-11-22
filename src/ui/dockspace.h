#pragma once

#include "ui/documents.h"
#include "ui/autosave_restore.h"
#include "ui/taskbar.h"
#include "ui/animation_preview.h"
#include "ui/panel.h"
#include "ui/shaders.h"
#include "ui/spritesheet_editor.h"
#include "ui/timeline.h"

namespace anm2ed::imgui
{
  class Dockspace
  {
    AnimationPreview animationPreview;
    bool isCanvasFocused{};
    float reloadTime{};
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
    PanelState libraries{};
    Timeline timeline;
    PopupHelper toolsColorEditPopup{PopupHelper(LABEL_TOOLS_COLOR_EDIT_POPUP, POPUP_TO_CONTENT, POPUP_BY_ITEM)};
    AutosaveRestore autosaveRestore;

  public:
    bool is_canvas_focused_get() const;
    void tick(Manager&, Settings&, float);
    void update(Taskbar&, Documents&, Manager&, Settings&, Resources&, Dialog&, Clipboard&);
  };
}
