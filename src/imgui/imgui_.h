#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "strings.h"
#include "types.h"

namespace anm2ed::imgui
{
  constexpr auto DRAG_SPEED = 0.25f;
  constexpr auto DRAG_SPEED_FAST = 1.00f;
  constexpr auto STEP = 1.0f;
  constexpr auto STEP_FAST = 5.0f;

#define POPUP_LIST                                                                                                     \
  X(POPUP_SMALL, 0.25f, true)                                                                                          \
  X(POPUP_NORMAL, 0.5f, true)                                                                                          \
  X(POPUP_TO_CONTENT, 0.0f, true)                                                                                      \
  X(POPUP_SMALL_NO_HEIGHT, 0.25f, false)                                                                               \
  X(POPUP_NORMAL_NO_HEIGHT, 0.5f, false)

  enum PopupType
  {
#define X(name, multiplier, isHeightSet) name,
    POPUP_LIST
#undef X
  };

  enum PopupPosition
  {
    POPUP_CENTER,
    POPUP_BY_ITEM,
    POPUP_BY_CURSOR
  };

  enum RenameState
  {
    RENAME_SELECTABLE,
    RENAME_BEGIN,
    RENAME_EDITING,
    RENAME_FINISHED,
    RENAME_FORCE_EDIT
  };

  constexpr float POPUP_MULTIPLIERS[] = {
#define X(name, multiplier, isHeightSet) multiplier,
      POPUP_LIST
#undef X
  };

  constexpr bool POPUP_IS_HEIGHT_SET[] = {
#define X(name, multiplier, isHeightSet) isHeightSet,
      POPUP_LIST
#undef X
  };

  const std::unordered_map<std::string, ImGuiKey> KEY_MAP = {
      {"A", ImGuiKey_A},
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
  };

  const std::unordered_map<std::string, ImGuiKey> MOD_MAP = {
      {"Ctrl", ImGuiMod_Ctrl},
      {"Shift", ImGuiMod_Shift},
      {"Alt", ImGuiMod_Alt},
      {"Super", ImGuiMod_Super},
  };

  void theme_set(types::theme::Type theme);
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
  bool input_text_path(const char*, std::filesystem::path*, ImGuiInputTextFlags = 0);
  bool input_int_range(const char*, int&, int, int, int = STEP, int = STEP_FAST, ImGuiInputTextFlags = 0);
  bool input_int2_range(const char*, glm::ivec2&, glm::ivec2, glm::ivec2, ImGuiInputTextFlags = 0);
  bool input_float_range(const char*, float&, float, float, float = STEP, float = STEP_FAST, const char* = "%.3f",
                         ImGuiInputTextFlags = 0);
  types::edit::Type drag_int_persistent(const char*, int*, float = DRAG_SPEED, int = {}, int = {}, const char* = "%d",
                                        ImGuiSliderFlags = 0);
  types::edit::Type drag_float_persistent(const char*, float*, float = DRAG_SPEED, float = {}, float = {},
                                          const char* = "%.3f", ImGuiSliderFlags = 0);
  types::edit::Type drag_float2_persistent(const char*, glm::vec2*, float = DRAG_SPEED, float = {}, float = {},
                                           const char* = "%.3f", ImGuiSliderFlags = 0);
  types::edit::Type color_edit3_persistent(const char*, glm::vec3*, ImGuiColorEditFlags = 0);
  types::edit::Type color_edit4_persistent(const char*, glm::vec4*, ImGuiColorEditFlags = 0);
  bool combo_negative_one_indexed(const std::string&, int*, std::vector<const char*>&);
  std::string& selectable_input_text_id();
  bool selectable_input_text(const std::string& label, const std::string& id, std::string& text, bool isSelected,
                             ImGuiSelectableFlags flags, RenameState& state);
  void set_item_tooltip_shortcut(const char*, const std::string& = {});
  void external_storage_set(ImGuiSelectionExternalStorage*, int, bool);
  void render_checker_background(ImDrawList*, ImVec2, ImVec2, glm::vec2, float);
  ImVec2 icon_size_get();
  bool shortcut(ImGuiKeyChord, types::shortcut::Type = types::shortcut::FOCUSED_SET);

  class MultiSelectStorage : public std::set<int>
  {
  public:
    ImGuiSelectionExternalStorage internal{};
    ImGuiMultiSelectIO* io{};
    std::vector<int>* indexMap{};

    using std::set<int>::set;
    using std::set<int>::operator=;
    using std::set<int>::begin;
    using std::set<int>::rbegin;
    using std::set<int>::end;
    using std::set<int>::size;
    using std::set<int>::insert;
    using std::set<int>::erase;

    MultiSelectStorage();
    void start(size_t,
               ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ScopeWindow);
    void apply();
    void finish();
    void set_index_map(std::vector<int>*);
    int resolve_index(int) const;
  };

  class PopupHelper
  {
  public:
    StringType labelId{};
    PopupType type{};
    PopupPosition position{};
    bool isOpen{};
    bool isTriggered{};
    bool isJustOpened{};

    PopupHelper(StringType, PopupType = POPUP_NORMAL, PopupPosition = POPUP_CENTER);
    const char* label() const { return localize.get(labelId); }
    bool is_open();
    void open();
    void trigger();
    void end();
    void close();
  };
}
