#include "shortcut.hpp"

#include <sstream>
#include <utility>
#include <vector>

#include <imgui/imgui_internal.h>

namespace anm2ed::imgui
{
  const std::vector<std::pair<ImGuiKey, const char*>> CANONICAL_KEY_NAMES = {
      {ImGuiKey_A, "A"},
      {ImGuiKey_B, "B"},
      {ImGuiKey_C, "C"},
      {ImGuiKey_D, "D"},
      {ImGuiKey_E, "E"},
      {ImGuiKey_F, "F"},
      {ImGuiKey_G, "G"},
      {ImGuiKey_H, "H"},
      {ImGuiKey_I, "I"},
      {ImGuiKey_J, "J"},
      {ImGuiKey_K, "K"},
      {ImGuiKey_L, "L"},
      {ImGuiKey_M, "M"},
      {ImGuiKey_N, "N"},
      {ImGuiKey_O, "O"},
      {ImGuiKey_P, "P"},
      {ImGuiKey_Q, "Q"},
      {ImGuiKey_R, "R"},
      {ImGuiKey_S, "S"},
      {ImGuiKey_T, "T"},
      {ImGuiKey_U, "U"},
      {ImGuiKey_V, "V"},
      {ImGuiKey_W, "W"},
      {ImGuiKey_X, "X"},
      {ImGuiKey_Y, "Y"},
      {ImGuiKey_Z, "Z"},
      {ImGuiKey_0, "0"},
      {ImGuiKey_1, "1"},
      {ImGuiKey_2, "2"},
      {ImGuiKey_3, "3"},
      {ImGuiKey_4, "4"},
      {ImGuiKey_5, "5"},
      {ImGuiKey_6, "6"},
      {ImGuiKey_7, "7"},
      {ImGuiKey_8, "8"},
      {ImGuiKey_9, "9"},
      {ImGuiKey_Keypad0, "Num0"},
      {ImGuiKey_Keypad1, "Num1"},
      {ImGuiKey_Keypad2, "Num2"},
      {ImGuiKey_Keypad3, "Num3"},
      {ImGuiKey_Keypad4, "Num4"},
      {ImGuiKey_Keypad5, "Num5"},
      {ImGuiKey_Keypad6, "Num6"},
      {ImGuiKey_Keypad7, "Num7"},
      {ImGuiKey_Keypad8, "Num8"},
      {ImGuiKey_Keypad9, "Num9"},
      {ImGuiKey_KeypadAdd, "NumAdd"},
      {ImGuiKey_KeypadSubtract, "NumSubtract"},
      {ImGuiKey_KeypadMultiply, "NumMultiply"},
      {ImGuiKey_KeypadDivide, "NumDivide"},
      {ImGuiKey_KeypadEnter, "NumEnter"},
      {ImGuiKey_KeypadDecimal, "NumDecimal"},
      {ImGuiKey_KeypadEqual, "NumEqual"},
      {ImGuiKey_F1, "F1"},
      {ImGuiKey_F2, "F2"},
      {ImGuiKey_F3, "F3"},
      {ImGuiKey_F4, "F4"},
      {ImGuiKey_F5, "F5"},
      {ImGuiKey_F6, "F6"},
      {ImGuiKey_F7, "F7"},
      {ImGuiKey_F8, "F8"},
      {ImGuiKey_F9, "F9"},
      {ImGuiKey_F10, "F10"},
      {ImGuiKey_F11, "F11"},
      {ImGuiKey_F12, "F12"},
      {ImGuiKey_UpArrow, "Up"},
      {ImGuiKey_DownArrow, "Down"},
      {ImGuiKey_LeftArrow, "Left"},
      {ImGuiKey_RightArrow, "Right"},
      {ImGuiKey_Space, "Space"},
      {ImGuiKey_Enter, "Enter"},
      {ImGuiKey_Escape, "Escape"},
      {ImGuiKey_Tab, "Tab"},
      {ImGuiKey_Backspace, "Backspace"},
      {ImGuiKey_Delete, "Delete"},
      {ImGuiKey_Insert, "Insert"},
      {ImGuiKey_Home, "Home"},
      {ImGuiKey_End, "End"},
      {ImGuiKey_PageUp, "PageUp"},
      {ImGuiKey_PageDown, "PageDown"},
      {ImGuiKey_Minus, "Minus"},
      {ImGuiKey_Equal, "Equal"},
      {ImGuiKey_LeftBracket, "LeftBracket"},
      {ImGuiKey_RightBracket, "RightBracket"},
      {ImGuiKey_Semicolon, "Semicolon"},
      {ImGuiKey_Apostrophe, "Apostrophe"},
      {ImGuiKey_Comma, "Comma"},
      {ImGuiKey_Period, "Period"},
      {ImGuiKey_Slash, "Slash"},
      {ImGuiKey_Backslash, "Backslash"},
      {ImGuiKey_GraveAccent, "GraveAccent"},
  };

  const char* shortcut_canonical_key_name(ImGuiKey key)
  {
    for (const auto& [mappedKey, name] : CANONICAL_KEY_NAMES)
      if (mappedKey == key) return name;
    return nullptr;
  }

  std::string chord_to_string(ImGuiKeyChord chord)
  {
    std::string result;

    if (chord & ImGuiMod_Ctrl) result += "Ctrl+";
    if (chord & ImGuiMod_Shift) result += "Shift+";
    if (chord & ImGuiMod_Alt) result += "Alt+";
    if (chord & ImGuiMod_Super) result += "Super+";

    if (auto key = (ImGuiKey)(chord & ~ImGuiMod_Mask_); key != ImGuiKey_None)
    {
      if (const char* name = shortcut_canonical_key_name(key); name && *name)
        result += name;
      else
        result += ImGui::GetKeyName(key);
    }

    if (!result.empty() && result.back() == '+') result.pop_back();

    return result;
  }

  ImGuiKeyChord string_to_chord(const std::string& string)
  {
    ImGuiKeyChord chord = 0;
    ImGuiKey baseKey = ImGuiKey_None;

    std::stringstream ss(string);
    std::string token;
    while (std::getline(ss, token, '+'))
    {
      token.erase(0, token.find_first_not_of(" \t\r\n"));
      token.erase(token.find_last_not_of(" \t\r\n") + 1);

      if (token.empty()) continue;

      if (auto it = MOD_MAP.find(token); it != MOD_MAP.end())
        chord |= it->second;
      else if (baseKey == ImGuiKey_None)
        if (auto it2 = KEY_MAP.find(token); it2 != KEY_MAP.end()) baseKey = it2->second;
    }

    if (baseKey != ImGuiKey_None) chord |= baseKey;

    return chord;
  }

  bool shortcut(ImGuiKeyChord chord, types::shortcut::Type type)
  {
    if (chord == ImGuiKey_None) return false;

    if (ImGui::GetTopMostPopupModal() != nullptr &&
        (type == types::shortcut::GLOBAL || type == types::shortcut::GLOBAL_SET))
      return false;

    int flags = type == types::shortcut::GLOBAL || type == types::shortcut::GLOBAL_SET ? ImGuiInputFlags_RouteGlobal
                                                                                       : ImGuiInputFlags_RouteFocused;
    flags |= ImGuiInputFlags_Repeat;

    if (type == types::shortcut::GLOBAL_SET || type == types::shortcut::FOCUSED_SET)
    {
      ImGui::SetNextItemShortcut(chord, flags);
      return false;
    }

    return ImGui::Shortcut(chord, flags);
  }
}
