#pragma once

#include <imgui/imgui.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "types.h"

namespace anm2ed::imgui
{
  constexpr auto POPUP_TO_CONTENT = 0.0f;
  constexpr auto POPUP_SMALL = 0.25f;
  constexpr auto POPUP_NORMAL = 0.5f;

  const std::unordered_map<std::string, ImGuiKey> KEY_MAP = {{"A", ImGuiKey_A},
                                                             {"B", ImGuiKey_B},
                                                             {"C", ImGuiKey_C},
                                                             {"D", ImGuiKey_D},
                                                             {"E", ImGuiKey_E},
                                                             {"F", ImGuiKey_F},
                                                             {"G", ImGuiKey_G},
                                                             {"H", ImGuiKey_H},
                                                             {"I", ImGuiKey_I},
                                                             {"J", ImGuiKey_J},
                                                             {"K", ImGuiKey_K},
                                                             {"L", ImGuiKey_L},
                                                             {"M", ImGuiKey_M},
                                                             {"N", ImGuiKey_N},
                                                             {"O", ImGuiKey_O},
                                                             {"P", ImGuiKey_P},
                                                             {"Q", ImGuiKey_Q},
                                                             {"R", ImGuiKey_R},
                                                             {"S", ImGuiKey_S},
                                                             {"T", ImGuiKey_T},
                                                             {"U", ImGuiKey_U},
                                                             {"V", ImGuiKey_V},
                                                             {"W", ImGuiKey_W},
                                                             {"X", ImGuiKey_X},
                                                             {"Y", ImGuiKey_Y},
                                                             {"Z", ImGuiKey_Z},

                                                             {"0", ImGuiKey_0},
                                                             {"1", ImGuiKey_1},
                                                             {"2", ImGuiKey_2},
                                                             {"3", ImGuiKey_3},
                                                             {"4", ImGuiKey_4},
                                                             {"5", ImGuiKey_5},
                                                             {"6", ImGuiKey_6},
                                                             {"7", ImGuiKey_7},
                                                             {"8", ImGuiKey_8},
                                                             {"9", ImGuiKey_9},

                                                             {"Num0", ImGuiKey_Keypad0},
                                                             {"Num1", ImGuiKey_Keypad1},
                                                             {"Num2", ImGuiKey_Keypad2},
                                                             {"Num3", ImGuiKey_Keypad3},
                                                             {"Num4", ImGuiKey_Keypad4},
                                                             {"Num5", ImGuiKey_Keypad5},
                                                             {"Num6", ImGuiKey_Keypad6},
                                                             {"Num7", ImGuiKey_Keypad7},
                                                             {"Num8", ImGuiKey_Keypad8},
                                                             {"Num9", ImGuiKey_Keypad9},
                                                             {"NumAdd", ImGuiKey_KeypadAdd},
                                                             {"NumSubtract", ImGuiKey_KeypadSubtract},
                                                             {"NumMultiply", ImGuiKey_KeypadMultiply},
                                                             {"NumDivide", ImGuiKey_KeypadDivide},
                                                             {"NumEnter", ImGuiKey_KeypadEnter},
                                                             {"NumDecimal", ImGuiKey_KeypadDecimal},

                                                             {"F1", ImGuiKey_F1},
                                                             {"F2", ImGuiKey_F2},
                                                             {"F3", ImGuiKey_F3},
                                                             {"F4", ImGuiKey_F4},
                                                             {"F5", ImGuiKey_F5},
                                                             {"F6", ImGuiKey_F6},
                                                             {"F7", ImGuiKey_F7},
                                                             {"F8", ImGuiKey_F8},
                                                             {"F9", ImGuiKey_F9},
                                                             {"F10", ImGuiKey_F10},
                                                             {"F11", ImGuiKey_F11},
                                                             {"F12", ImGuiKey_F12},

                                                             {"Up", ImGuiKey_UpArrow},
                                                             {"Down", ImGuiKey_DownArrow},
                                                             {"Left", ImGuiKey_LeftArrow},
                                                             {"Right", ImGuiKey_RightArrow},

                                                             {"Space", ImGuiKey_Space},
                                                             {"Enter", ImGuiKey_Enter},
                                                             {"Escape", ImGuiKey_Escape},
                                                             {"Tab", ImGuiKey_Tab},
                                                             {"Backspace", ImGuiKey_Backspace},
                                                             {"Delete", ImGuiKey_Delete},
                                                             {"Insert", ImGuiKey_Insert},
                                                             {"Home", ImGuiKey_Home},
                                                             {"End", ImGuiKey_End},
                                                             {"PageUp", ImGuiKey_PageUp},
                                                             {"PageDown", ImGuiKey_PageDown},

                                                             {"Minus", ImGuiKey_Minus},
                                                             {"Equal", ImGuiKey_Equal},
                                                             {"LeftBracket", ImGuiKey_LeftBracket},
                                                             {"RightBracket", ImGuiKey_RightBracket},
                                                             {"Semicolon", ImGuiKey_Semicolon},
                                                             {"Apostrophe", ImGuiKey_Apostrophe},
                                                             {"Comma", ImGuiKey_Comma},
                                                             {"Period", ImGuiKey_Period},
                                                             {"Slash", ImGuiKey_Slash},
                                                             {"Backslash", ImGuiKey_Backslash},
                                                             {"GraveAccent", ImGuiKey_GraveAccent},

                                                             {"MouseLeft", ImGuiKey_MouseLeft},
                                                             {"MouseRight", ImGuiKey_MouseRight},
                                                             {"MouseMiddle", ImGuiKey_MouseMiddle},
                                                             {"MouseX1", ImGuiKey_MouseX1},
                                                             {"MouseX2", ImGuiKey_MouseX2}};

  const std::unordered_map<std::string, ImGuiKey> MOD_MAP = {
      {"Ctrl", ImGuiMod_Ctrl},
      {"Shift", ImGuiMod_Shift},
      {"Alt", ImGuiMod_Alt},
      {"Super", ImGuiMod_Super},
  };

  std::string chord_to_string(ImGuiKeyChord);
  ImGuiKeyChord string_to_chord(const std::string&);
  float row_widget_width_get(int, float = ImGui::GetContentRegionAvail().x);
  ImVec2 widget_size_with_row_get(int, float = ImGui::GetContentRegionAvail().x);
  float footer_height_get(int = 1);
  ImVec2 footer_size_get(int = 1);
  ImVec2 size_without_footer_get(int = 1);
  ImVec2 child_size_get(int = 1);
  int input_text_callback(ImGuiInputTextCallbackData*);
  bool input_text_string(const char*, std::string*, ImGuiInputTextFlags = 0);
  void combo_strings(const std::string&, int*, std::vector<std::string>&);
  void combo_strings(const std::string&, int*, std::vector<const char*>&);
  bool selectable_input_text(const std::string&, const std::string&, std::string&, bool = false,
                             ImGuiSelectableFlags = 0, bool* = nullptr);
  void set_item_tooltip_shortcut(const char*, const std::string& = {});
  void external_storage_set(ImGuiSelectionExternalStorage*, int, bool);
  ImVec2 icon_size_get();
  bool chord_held(ImGuiKeyChord);
  bool chord_repeating(ImGuiKeyChord, float = 0.125f, float = 0.025f);
  bool shortcut(std::string, types::shortcut::Type = types::shortcut::FOCUSED_SET);

  class MultiSelectStorage : public std::set<int>
  {
  public:
    ImGuiSelectionExternalStorage internal{};
    using std::set<int>::set;
    using std::set<int>::operator=;
    using std::set<int>::begin;
    using std::set<int>::rbegin;
    using std::set<int>::end;
    using std::set<int>::size;
    using std::set<int>::insert;
    using std::set<int>::erase;

    MultiSelectStorage();
    void start(size_t);
    void finish();
  };

  class PopupHelper
  {
  public:
    const char* label{};
    bool isOpen{};
    bool isTriggered{};
    bool isJustOpened{};
    bool isNoHeight{};
    float percent{};

    PopupHelper(const char*, float = POPUP_NORMAL, bool = false);
    bool is_open();
    void open();
    void trigger();
    void end();
    void close();
  };
}
