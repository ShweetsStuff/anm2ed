#pragma once

#include <imgui/imgui.h>

#include "icon.h"
#include "settings.h"
#include "localization.h"

  enum Tool
  {
    TOOL_PAN,
    TOOL_MOVE,
    TOOL_ROTATE,
    TOOL_SCALE,
    TOOL_SKEW,
    TOOL_CROP,
    TOOL_DRAW,
    TOOL_ERASE,
    TOOL_COLOR_PICKER,
    TOOL_UNDO,
    TOOL_REDO,
    TOOL_COLOR,
    TOOL_COUNT
  };

  enum ToolArea
  {
    TOOL_AREA_ANIMATION_PREVIEW,
    TOOL_AREA_SPRITESHEET_EDITOR,
    TOOL_AREA_ALL
  };

  struct ToolInfo
  {
    ImGuiMouseCursor cursor = ImGuiMouseCursor_None;
    Icon icon = {};
    ShortcutType shortcut = {};
    ToolArea areaType;
    const char* label = nullptr;
    StringType tooltip = {};
    bool isFrameRequired = false;
    bool isSpritesheetRequired = false;
  };

  constexpr ToolInfo TOOLS[] = {
      {ImGuiMouseCursor_Hand, ICON_PAN, SHORTCUT_PAN, TOOL_AREA_ALL, "##Pan", TOOLTIP_TOOL_PAN},

      {ImGuiMouseCursor_ResizeAll, ICON_MOVE, SHORTCUT_MOVE, TOOL_AREA_ALL, "##Move", TOOLTIP_TOOL_MOVE, true},

      {ImGuiMouseCursor_Arrow, ICON_ROTATE, SHORTCUT_ROTATE, TOOL_AREA_ANIMATION_PREVIEW, "##Rotate",
       TOOLTIP_TOOL_ROTATE, true},

      {ImGuiMouseCursor_ResizeNESW, ICON_SCALE, SHORTCUT_SCALE, TOOL_AREA_ANIMATION_PREVIEW, "##Scale",
       TOOLTIP_TOOL_SCALE, true},

      {ImGuiMouseCursor_ResizeNESW, ICON_SKEW, SHORTCUT_SKEW, TOOL_AREA_ANIMATION_PREVIEW, "##Skew", TOOLTIP_TOOL_SKEW,
       true},

      {ImGuiMouseCursor_Arrow, ICON_CROP, SHORTCUT_CROP, TOOL_AREA_SPRITESHEET_EDITOR, "##Crop", TOOLTIP_TOOL_CROP,
       true},

      {ImGuiMouseCursor_Arrow, ICON_DRAW, SHORTCUT_DRAW, TOOL_AREA_SPRITESHEET_EDITOR, "##Draw", TOOLTIP_TOOL_DRAW,
       false, true},

      {ImGuiMouseCursor_Arrow, ICON_ERASE, SHORTCUT_ERASE, TOOL_AREA_SPRITESHEET_EDITOR, "##Erase", TOOLTIP_TOOL_ERASE,
       false, true},

      {ImGuiMouseCursor_Arrow, ICON_COLOR_PICKER, SHORTCUT_COLOR_PICKER, TOOL_AREA_SPRITESHEET_EDITOR,
       "##Color Picker", TOOLTIP_TOOL_COLOR_PICKER, false, true},

      {ImGuiMouseCursor_None, ICON_UNDO, SHORTCUT_UNDO, TOOL_AREA_ALL, "##Undo", TOOLTIP_TOOL_UNDO},

      {ImGuiMouseCursor_None, ICON_REDO, SHORTCUT_REDO, TOOL_AREA_ALL, "##Redo", TOOLTIP_TOOL_REDO},

      {ImGuiMouseCursor_None, ICON_NONE, SHORTCUT_COLOR, TOOL_AREA_ALL, "##Color", TOOLTIP_TOOL_COLOR},
  };
