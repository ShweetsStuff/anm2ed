#pragma once

#include "icon.h"
#include "settings.h"

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
    const char* tooltip{};
  };

  constexpr Info INFO[] = {
      {ImGuiMouseCursor_Hand, resource::icon::PAN, SHORTCUT_PAN, ALL, "##Pan",
       "Use the pan tool.\nWill shift the view as the cursor is dragged.\nYou can also use the middle mouse button to "
       "pan at any time."},

      {ImGuiMouseCursor_ResizeAll, resource::icon::MOVE, SHORTCUT_MOVE, ALL, "##Move",
       "Use the move tool.\nAnimation Preview: Will move the position of the frame."
       "\nSpritesheet Editor: Will move the pivot, and holding right click will use the Crop functionality instead."
       "\nUse mouse or directional keys to change the value."},

      {ImGuiMouseCursor_Arrow, resource::icon::ROTATE, SHORTCUT_ROTATE, ANIMATION_PREVIEW, "##Rotate",
       "Use the rotate tool.\nWill rotate the selected item as the cursor is dragged, or directional keys are "
       "pressed.\n(Animation Preview only.)"},

      {ImGuiMouseCursor_ResizeNESW, resource::icon::SCALE, SHORTCUT_SCALE, ANIMATION_PREVIEW, "##Scale",
       "Use the scale tool.\nWill scale the selected item as the cursor is dragged, or directional keys are "
       "pressed.\nHold SHIFT to lock scaling to one dimension.\n(Animation Preview only.)"},

      {ImGuiMouseCursor_Arrow, resource::icon::CROP, SHORTCUT_CROP, SPRITESHEET_EDITOR, "##Crop",
       "Use the crop tool.\nWill produce a crop rectangle based on how the cursor is dragged, or directional keys are "
       "pressed.\nHold CTRL with arrow keys to change position."
       "\nHolding right click will use the Move tool's functionality."
       "\n(Spritesheet Editor only.)"},

      {ImGuiMouseCursor_Arrow, resource::icon::DRAW, SHORTCUT_DRAW, SPRITESHEET_EDITOR, "##Draw",
       "Draws pixels onto the selected spritesheet, with the current color.\n(Spritesheet Editor only.)"},

      {ImGuiMouseCursor_Arrow, resource::icon::ERASE, SHORTCUT_ERASE, SPRITESHEET_EDITOR, "##Erase",
       "Erases pixels from the selected spritesheet.\n(Spritesheet Editor only.)"},

      {ImGuiMouseCursor_Arrow, resource::icon::COLOR_PICKER, SHORTCUT_COLOR_PICKER, SPRITESHEET_EDITOR,
       "##Color Picker", "Selects a color from the canvas.\n(Spritesheet Editor only.)"},

      {ImGuiMouseCursor_None, resource::icon::UNDO, SHORTCUT_UNDO, ALL, "##Undo", "Undoes the last action."},

      {ImGuiMouseCursor_None, resource::icon::REDO, SHORTCUT_REDO, ALL, "##Redo", "Redoes the last action."},

      {ImGuiMouseCursor_None, resource::icon::NONE, SHORTCUT_COLOR, ALL, "##Color",
       "Selects the color to be used for drawing.\n(Spritesheet Editor only.)"},
  };
}