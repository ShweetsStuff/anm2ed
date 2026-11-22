#pragma once

#include <string>

#include <imgui/imgui.h>

#include "types.h"

// Keyboard keys a shortcut can use: Tab through the keypad, without the modifier keys themselves.
bool is_shortcut_key(ImGuiKey);
std::string chord_to_string(ImGuiKeyChord);
ImGuiKeyChord string_to_chord(const std::string&);
bool shortcut(ImGuiKeyChord, ShortcutScope = SCOPE_FOCUSED_SET);
