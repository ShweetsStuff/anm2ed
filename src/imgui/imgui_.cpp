#include "imgui_.h"

#include <imgui/imgui_internal.h>

#include <set>
#include <sstream>
#include <unordered_map>

using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::imgui
{
  int input_text_callback(ImGuiInputTextCallbackData* data)
  {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
      auto* string = (std::string*)(data->UserData);
      string->resize(data->BufTextLen);
      data->Buf = string->data();
    }
    return 0;
  }

  bool input_text_string(const char* label, std::string* string, ImGuiInputTextFlags flags)
  {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputText(label, string->data(), string->capacity() + 1, flags, input_text_callback, string);
  }

  bool combo_strings(const std::string& label, int* index, std::vector<std::string>& strings)
  {
    std::vector<const char*> items{};
    for (auto& string : strings)
      items.push_back(string.c_str());
    return ImGui::Combo(label.c_str(), index, items.data(), (int)items.size());
  }

  bool combo_strings(const std::string& label, int* index, std::vector<const char*>& strings)
  {
    return ImGui::Combo(label.c_str(), index, strings.data(), (int)strings.size());
  }

  bool input_int_range(const char* label, int& value, int min, int max, int step, int stepFast,
                       ImGuiInputTextFlags flags)
  {
    auto isActivated = ImGui::InputInt(label, &value, step, stepFast, flags);
    value = glm::clamp(value, min, max);
    return isActivated;
  }

  bool selectable_input_text(const std::string& label, const std::string& id, std::string& text, bool isSelected,
                             ImGuiSelectableFlags flags, bool* isRenamed)
  {
    static std::string editID{};
    static bool isJustEdit{};
    const bool isEditing = editID == id;
    bool isActivated{};

    if (isEditing)
    {
      if (isJustEdit)
      {
        ImGui::SetKeyboardFocusHere();
        isJustEdit = false;
      }

      ImGui::SetNextItemWidth(-FLT_MIN);
      if (input_text_string("##Edit", &text, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
      {
        editID.clear();
        isActivated = true;
        if (isRenamed) *isRenamed = true;
      }
      if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsKeyPressed(ImGuiKey_Escape)) editID.clear();
    }
    else
    {
      if (ImGui::Selectable(label.c_str(), isSelected, flags)) isActivated = true;

      if ((ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F2) && isSelected) ||
          (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)))
      {
        editID = id;
        isJustEdit = true;
      }
    }

    return isActivated;
  }

  void set_item_tooltip_shortcut(const char* tooltip, const std::string& shortcut)
  {
    ImGui::SetItemTooltip("%s\n(Shortcut: %s)", tooltip, shortcut.c_str());
  }

  void external_storage_set(ImGuiSelectionExternalStorage* self, int id, bool isSelected)
  {
    auto* set = (std::set<int>*)self->UserData;
    if (isSelected)
      set->insert(id);
    else
      set->erase(id);
  };

  std::string chord_to_string(ImGuiKeyChord chord)
  {
    std::string result;

    if (chord & ImGuiMod_Ctrl) result += "Ctrl+";
    if (chord & ImGuiMod_Shift) result += "Shift+";
    if (chord & ImGuiMod_Alt) result += "Alt+";
    if (chord & ImGuiMod_Super) result += "Super+";

    if (auto key = (ImGuiKey)(chord & ~ImGuiMod_Mask_); key != ImGuiKey_None)
    {
      if (const char* name = ImGui::GetKeyName(key); name && *name)
        result += name;
      else
        result += "Unknown";
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

  float row_widget_width_get(int count, float width)
  {
    return (width - (ImGui::GetStyle().ItemSpacing.x * (float)(count - 1))) / (float)count;
  }

  ImVec2 widget_size_with_row_get(int count, float width)
  {
    return ImVec2(row_widget_width_get(count, width), 0);
  }

  float footer_height_get(int itemCount)
  {
    return ImGui::GetTextLineHeightWithSpacing() * itemCount + ImGui::GetStyle().WindowPadding.y +
           ImGui::GetStyle().ItemSpacing.y * (itemCount);
  }

  ImVec2 footer_size_get(int itemCount)
  {
    return ImVec2(ImGui::GetContentRegionAvail().x, footer_height_get(itemCount));
  }

  ImVec2 size_without_footer_get(int rowCount)
  {
    return ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - footer_height_get(rowCount));
  }

  ImVec2 child_size_get(int rowCount)
  {
    return ImVec2(ImGui::GetContentRegionAvail().x,
                  (ImGui::GetFrameHeightWithSpacing() * rowCount) + (ImGui::GetStyle().WindowPadding.y * 2.0f));
  }

  ImVec2 icon_size_get()
  {
    return ImVec2(ImGui::GetTextLineHeightWithSpacing(), ImGui::GetTextLineHeightWithSpacing());
  }

  bool chord_held(ImGuiKeyChord chord)
  {
    auto& io = ImGui::GetIO();

    for (constexpr ImGuiKey mods[] = {ImGuiMod_Ctrl, ImGuiMod_Shift, ImGuiMod_Alt, ImGuiMod_Super}; ImGuiKey mod : mods)
    {
      bool required = (chord & mod) != 0;
      if (bool held = io.KeyMods & mod; required && !held) return false;
    }

    auto main_key = (ImGuiKey)(chord & ~ImGuiMod_Mask_);
    if (main_key == ImGuiKey_None) return false;

    return ImGui::IsKeyDown(main_key);
  }

  bool chord_repeating(ImGuiKeyChord chord, float delay, float rate)
  {
    struct State
    {
      float timeHeld = 0.f;
      float nextRepeat = 0.f;
    };
    static std::unordered_map<ImGuiKeyChord, State> stateMap;

    auto& io = ImGui::GetIO();
    auto& state = stateMap[chord];

    if (chord_held(chord))
    {
      state.timeHeld += io.DeltaTime;

      if (state.timeHeld <= io.DeltaTime)
      {
        state.nextRepeat = delay;
        return true;
      }

      if (state.timeHeld >= state.nextRepeat)
      {
        state.nextRepeat += rate;
        return true;
      }
    }
    else
    {
      state.timeHeld = 0.f;
      state.nextRepeat = 0.f;
    }

    return false;
  }

  bool shortcut(std::string string, shortcut::Type type)
  {
    if (ImGui::GetTopMostPopupModal() != nullptr) return false;
    int flags = type == shortcut::GLOBAL || type == shortcut::GLOBAL_SET ? ImGuiInputFlags_RouteGlobal
                                                                         : ImGuiInputFlags_RouteFocused;
    if (type == shortcut::GLOBAL_SET || type == shortcut::FOCUSED_SET)
    {
      ImGui::SetNextItemShortcut(string_to_chord(string), flags);
      return false;
    }

    return ImGui::Shortcut(string_to_chord(string), flags);
  }

  MultiSelectStorage::MultiSelectStorage()
  {
    internal.AdapterSetItemSelected = external_storage_set;
  }

  void MultiSelectStorage::start(size_t size)
  {
    internal.UserData = this;

    auto io = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_ClearOnEscape, this->size(), size);
    internal.ApplyRequests(io);
  }

  void MultiSelectStorage::finish()
  {
    auto io = ImGui::EndMultiSelect();
    internal.ApplyRequests(io);
  }

  PopupHelper::PopupHelper(const char* label, PopupType type, PopupPosition position)
  {
    this->label = label;
    this->type = type;
    this->position = position;
  }

  void PopupHelper::open()
  {
    isOpen = true;
    isTriggered = true;
    isJustOpened = true;
  }

  bool PopupHelper::is_open()
  {
    return isOpen;
  }

  void PopupHelper::trigger()
  {
    if (isTriggered) ImGui::OpenPopup(label);
    isTriggered = false;

    auto viewport = ImGui::GetMainViewport();

    if (position == POPUP_CENTER)
      ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_None, to_imvec2(vec2(0.5f)));
    else
      ImGui::SetNextWindowPos(ImGui::GetItemRectMin(), ImGuiCond_None);

    if (POPUP_IS_HEIGHT_SET[type])
      ImGui::SetNextWindowSize(to_imvec2(to_vec2(viewport->Size) * POPUP_MULTIPLIERS[type]));
    else
      ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * POPUP_MULTIPLIERS[type], 0));
  }

  void PopupHelper::end()
  {
    isJustOpened = false;
  }

  void PopupHelper::close()
  {
    isOpen = false;
  }
}
