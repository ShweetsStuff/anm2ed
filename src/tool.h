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

  struct Info
  {
    ImGuiMouseCursor cursor{ImGuiMouseCursor_None};
    icon::Type icon{};
    settings::ShortcutType shortcut{};
    const char* label{};
    const char* tooltip{};
  };

  constexpr Info INFO[] = {
      {ImGuiMouseCursor_Hand, icon::PAN, settings::SHORTCUT_PAN, "##Pan",
       "Use the pan tool.\nWill shift the view as the cursor is dragged.\nYou can also use the middle mouse button to "
       "pan at any time."},

      {ImGuiMouseCursor_ResizeAll, icon::MOVE, settings::SHORTCUT_MOVE, "##Move",
       "Use the move tool.\nAnimation Preview: Will move the position of the frame."
       "\nSpritesheet Editor: Will move the pivot, and holding right click will use the Crop functionality instead."
       "\nUse mouse or directional keys to change the value."},

      {ImGuiMouseCursor_Arrow, icon::ROTATE, settings::SHORTCUT_ROTATE, "##Rotate",
       "Use the rotate tool.\nWill rotate the selected item as the cursor is dragged, or directional keys are "
       "pressed.\n(Animation Preview only.)"},

      {ImGuiMouseCursor_ResizeNWSE, icon::SCALE, settings::SHORTCUT_SCALE, "##Scale",
       "Use the scale tool.\nWill scale the selected item as the cursor is dragged, or directional keys are "
       "pressed.\n(Animation Preview only.)"},

      {ImGuiMouseCursor_ResizeAll, icon::CROP, settings::SHORTCUT_CROP, "##Crop",
       "Use the crop tool.\nWill produce a crop rectangle based on how the cursor is dragged."
       "\nAlternatively, you can use the arrow keys and Ctrl/Shift to move the size/position, respectively."
       "\nHolding right click will use the Move tool's functionality."
       "\n(Spritesheet Editor only.)"},

      {ImGuiMouseCursor_Hand, icon::DRAW, settings::SHORTCUT_DRAW, "##Draw",
       "Draws pixels onto the selected spritesheet, with the current color.\n(Spritesheet Editor only.)"},

      {ImGuiMouseCursor_Arrow, icon::ERASE, settings::SHORTCUT_ERASE, "##Erase",
       "Erases pixels from the selected spritesheet.\n(Spritesheet Editor only.)"},

      {ImGuiMouseCursor_Arrow, icon::COLOR_PICKER, settings::SHORTCUT_COLOR_PICKER, "##Color Picker",
       "Selects a color from the canvas.\n(Spritesheet Editor only.)"},

      {ImGuiMouseCursor_None, icon::UNDO, settings::SHORTCUT_UNDO, "##Undo", "Undoes the last action."},

      {ImGuiMouseCursor_None, icon::REDO, settings::SHORTCUT_REDO, "##Redo", "Redoes the last action."},

      {ImGuiMouseCursor_None, icon::NONE, settings::SHORTCUT_COLOR, "##Color",
       "Selects the color to be used for drawing.\n(Spritesheet Editor only.)"},
  };
}