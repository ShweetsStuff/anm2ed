#pragma once

#include "icon.h"
#include "settings.h"
#include "strings.h"

namespace anm2ed::tool
{
  enum Type
  {
    PAN,
    MOVE,
    ROTATE,
    SCALE,
    CROP,
    DRAW,
    ERASE,
    COLOR_PICKER,
    UNDO,
    REDO,
    COLOR,
    COUNT
  };

  enum AreaType
  {
    ANIMATION_PREVIEW,
    SPRITESHEET_EDITOR,
    ALL
  };

  struct Info
  {
    ImGuiMouseCursor cursor{ImGuiMouseCursor_None};
    resource::icon::Type icon{};
    ShortcutType shortcut{};
    AreaType areaType;
    const char* label{};
    StringType tooltip{};
  };

  constexpr Info INFO[] = {
      {ImGuiMouseCursor_Hand, resource::icon::PAN, SHORTCUT_PAN, ALL, "##Pan", TOOLTIP_TOOL_PAN},

      {ImGuiMouseCursor_ResizeAll, resource::icon::MOVE, SHORTCUT_MOVE, ALL, "##Move", TOOLTIP_TOOL_MOVE},

      {ImGuiMouseCursor_Arrow, resource::icon::ROTATE, SHORTCUT_ROTATE, ANIMATION_PREVIEW, "##Rotate",
       TOOLTIP_TOOL_ROTATE},

      {ImGuiMouseCursor_ResizeNESW, resource::icon::SCALE, SHORTCUT_SCALE, ANIMATION_PREVIEW, "##Scale",
       TOOLTIP_TOOL_SCALE},

      {ImGuiMouseCursor_Arrow, resource::icon::CROP, SHORTCUT_CROP, SPRITESHEET_EDITOR, "##Crop", TOOLTIP_TOOL_CROP},

      {ImGuiMouseCursor_Arrow, resource::icon::DRAW, SHORTCUT_DRAW, SPRITESHEET_EDITOR, "##Draw",
       TOOLTIP_TOOL_DRAW},

      {ImGuiMouseCursor_Arrow, resource::icon::ERASE, SHORTCUT_ERASE, SPRITESHEET_EDITOR, "##Erase",
       TOOLTIP_TOOL_ERASE},

      {ImGuiMouseCursor_Arrow, resource::icon::COLOR_PICKER, SHORTCUT_COLOR_PICKER, SPRITESHEET_EDITOR, "##Color Picker",
       TOOLTIP_TOOL_COLOR_PICKER},

      {ImGuiMouseCursor_None, resource::icon::UNDO, SHORTCUT_UNDO, ALL, "##Undo", TOOLTIP_TOOL_UNDO},

      {ImGuiMouseCursor_None, resource::icon::REDO, SHORTCUT_REDO, ALL, "##Redo", TOOLTIP_TOOL_REDO},

      {ImGuiMouseCursor_None, resource::icon::NONE, SHORTCUT_COLOR, ALL, "##Color", TOOLTIP_TOOL_COLOR},
  };
}
