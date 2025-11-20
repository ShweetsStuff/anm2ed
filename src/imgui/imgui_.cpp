#include "imgui_.h"

#include <imgui/imgui_internal.h>

#include <cmath>
#include <set>
#include <sstream>
#include <unordered_map>

using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::imgui
{
  static auto isRenaming = false;

  constexpr ImVec4 COLOR_LIGHT_BUTTON{0.98f, 0.98f, 0.98f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TITLE_BG{0.78f, 0.78f, 0.78f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TITLE_BG_ACTIVE{0.64f, 0.64f, 0.64f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TITLE_BG_COLLAPSED{0.74f, 0.74f, 0.74f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TABLE_HEADER{0.78f, 0.78f, 0.78f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB{0.74f, 0.74f, 0.74f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_HOVERED{0.82f, 0.82f, 0.82f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_SELECTED{0.92f, 0.92f, 0.92f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_DIMMED{0.70f, 0.70f, 0.70f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_DIMMED_SELECTED{0.86f, 0.86f, 0.86f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_OVERLINE{0.55f, 0.55f, 0.55f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_DIMMED_OVERLINE{0.50f, 0.50f, 0.50f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_CHECK_MARK{0.0f, 0.0f, 0.0f, 1.0f};
  constexpr auto FRAME_BORDER_SIZE = 1.0f;

  void theme_set(theme::Type theme)
  {
    switch (theme)
    {
      case theme::LIGHT:
        ImGui::StyleColorsLight();
        break;
      case theme::DARK:
      default:
        ImGui::StyleColorsDark();
        break;
      case theme::CLASSIC:
        ImGui::StyleColorsClassic();
        break;
    }
    auto& style = ImGui::GetStyle();
    style.FrameBorderSize = FRAME_BORDER_SIZE;

    if (theme == theme::LIGHT)
    {
      auto& colors = style.Colors;
      colors[ImGuiCol_Button] = COLOR_LIGHT_BUTTON;
      colors[ImGuiCol_TitleBg] = COLOR_LIGHT_TITLE_BG;
      colors[ImGuiCol_TitleBgActive] = COLOR_LIGHT_TITLE_BG_ACTIVE;
      colors[ImGuiCol_TitleBgCollapsed] = COLOR_LIGHT_TITLE_BG_COLLAPSED;
      colors[ImGuiCol_TableHeaderBg] = COLOR_LIGHT_TABLE_HEADER;
      colors[ImGuiCol_Tab] = COLOR_LIGHT_TAB;
      colors[ImGuiCol_TabHovered] = COLOR_LIGHT_TAB_HOVERED;
      colors[ImGuiCol_TabSelected] = COLOR_LIGHT_TAB_SELECTED;
      colors[ImGuiCol_TabSelectedOverline] = COLOR_LIGHT_TAB_OVERLINE;
      colors[ImGuiCol_TabDimmed] = COLOR_LIGHT_TAB_DIMMED;
      colors[ImGuiCol_TabDimmedSelected] = COLOR_LIGHT_TAB_DIMMED_SELECTED;
      colors[ImGuiCol_TabDimmedSelectedOverline] = COLOR_LIGHT_TAB_DIMMED_OVERLINE;
      colors[ImGuiCol_CheckMark] = COLOR_LIGHT_CHECK_MARK;
    }
  }

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

  bool combo_negative_one_indexed(const std::string& label, int* index, std::vector<const char*>& strings)
  {
    *index += 1;
    bool isActivated = ImGui::Combo(label.c_str(), index, strings.data(), (int)strings.size());
    *index -= 1;

    return isActivated;
  }

  bool input_int_range(const char* label, int& value, int min, int max, int step, int stepFast,
                       ImGuiInputTextFlags flags)
  {
    auto isActivated = ImGui::InputInt(label, &value, step, stepFast, flags);
    value = glm::clamp(value, min, max);
    return isActivated;
  }

  bool input_int2_range(const char* label, ivec2& value, ivec2 min, ivec2 max, ImGuiInputTextFlags flags)
  {
    auto isActivated = ImGui::InputInt2(label, value_ptr(value), flags);
    value = glm::clamp(value, min, max);
    return isActivated;
  }

  bool input_float_range(const char* label, float& value, float min, float max, float step, float stepFast,
                         const char* format, ImGuiInputTextFlags flags)
  {
    auto isActivated = ImGui::InputFloat(label, &value, step, stepFast, format, flags);
    value = glm::clamp(value, min, max);
    return isActivated;
  }

  bool selectable_input_text(const std::string& label, const std::string& id, std::string& text, bool isSelected,
                             ImGuiSelectableFlags flags, RenameState& state)
  {
    static std::string editID{};
    auto isRename = editID == id;
    bool isActivated{};

    if (isRename)
    {
      auto finish = [&]()
      {
        editID.clear();
        isActivated = true;
        state = RENAME_FINISHED;
        isRenaming = false;
      };

      if (state == RENAME_BEGIN)
      {
        ImGui::SetKeyboardFocusHere();
        state = RENAME_EDITING;
      }

      ImGui::SetNextItemWidth(-FLT_MIN);
      if (input_text_string("##Edit", &text, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        finish();
      if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsKeyPressed(ImGuiKey_Escape)) finish();
    }
    else
    {
      if (ImGui::Selectable(label.c_str(), isSelected, flags)) isActivated = true;

      if (state == RENAME_FORCE_EDIT || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F2)) ||
          (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)))
      {
        state = RENAME_BEGIN;
        editID = id;
        isActivated = true;
        isRenaming = true;
      }
    }

    return isActivated;
  }

  void set_item_tooltip_shortcut(const char* tooltip, const std::string& shortcut)
  {
    ImGui::SetItemTooltip("%s\n(Shortcut: %s)", tooltip, shortcut.c_str());
  }

  namespace
  {
    struct CheckerStart
    {
      float position{};
      long long index{};
    };

    CheckerStart checker_start(float minCoord, float offset, float step)
    {
      float world = minCoord + offset;
      long long idx = static_cast<long long>(std::floor(world / step));
      float first = minCoord - (world - static_cast<float>(idx) * step);
      return {first, idx};
    }
  }

  void render_checker_background(ImDrawList* drawList, ImVec2 min, ImVec2 max, vec2 offset, float step)
  {
    if (!drawList || step <= 0.0f) return;

    const ImU32 colorLight = IM_COL32(204, 204, 204, 255);
    const ImU32 colorDark = IM_COL32(128, 128, 128, 255);

    auto [startY, rowIndex] = checker_start(min.y, offset.y, step);
    for (float y = startY; y < max.y; y += step, ++rowIndex)
    {
      float y1 = glm::max(y, min.y);
      float y2 = glm::min(y + step, max.y);
      if (y2 <= y1) continue;

      auto [startX, columnIndex] = checker_start(min.x, offset.x, step);
      for (float x = startX; x < max.x; x += step, ++columnIndex)
      {
        float x1 = glm::max(x, min.x);
        float x2 = glm::min(x + step, max.x);
        if (x2 <= x1) continue;

        bool isDark = ((rowIndex + columnIndex) & 1LL) != 0;
        drawList->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), isDark ? colorDark : colorLight);
      }
    }
  }

  void external_storage_set(ImGuiSelectionExternalStorage* self, int id, bool isSelected)
  {
    auto* storage = static_cast<MultiSelectStorage*>(self->UserData);
    auto value = storage ? storage->resolve_index(id) : id;
    if (isSelected)
      storage->insert(value);
    else
      storage->erase(value);
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

  ImVec2 widget_size_with_row_get(int count, float width) { return ImVec2(row_widget_width_get(count, width), 0); }

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

  bool shortcut(ImGuiKeyChord chord, shortcut::Type type, bool isRepeat)
  {
    if (ImGui::GetTopMostPopupModal() != nullptr) return false;
    if (isRepeat && !isRenaming) return chord_repeating(chord);

    int flags = type == shortcut::GLOBAL || type == shortcut::GLOBAL_SET ? ImGuiInputFlags_RouteGlobal
                                                                         : ImGuiInputFlags_RouteFocused;
    if (type == shortcut::GLOBAL_SET || type == shortcut::FOCUSED_SET)
    {
      ImGui::SetNextItemShortcut(chord, flags);
      return false;
    }

    return ImGui::Shortcut(chord, flags);
  }

  MultiSelectStorage::MultiSelectStorage() { internal.AdapterSetItemSelected = external_storage_set; }

  void MultiSelectStorage::start(size_t size, ImGuiMultiSelectFlags flags)
  {
    internal.UserData = this;

    io = ImGui::BeginMultiSelect(flags, this->size(), size);
    apply();
  }

  void MultiSelectStorage::apply() { internal.ApplyRequests(io); }

  void MultiSelectStorage::finish()
  {
    io = ImGui::EndMultiSelect();
    apply();
  }

  void MultiSelectStorage::set_index_map(std::vector<int>* map) { indexMap = map; }

  int MultiSelectStorage::resolve_index(int index) const
  {
    if (!indexMap) return index;
    if (index < 0 || index >= (int)indexMap->size()) return index;
    return (*indexMap)[index];
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

  bool PopupHelper::is_open() { return isOpen; }

  void PopupHelper::trigger()
  {
    if (isTriggered) ImGui::OpenPopup(label);
    isTriggered = false;

    auto viewport = ImGui::GetMainViewport();

    switch (position)
    {
      case POPUP_CENTER:
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_None, to_imvec2(vec2(0.5f)));
        if (POPUP_IS_HEIGHT_SET[type])
          ImGui::SetNextWindowSize(to_imvec2(to_vec2(viewport->Size) * POPUP_MULTIPLIERS[type]));
        else
          ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * POPUP_MULTIPLIERS[type], 0));
        break;
      case POPUP_BY_ITEM:
        ImGui::SetNextWindowPos(ImGui::GetItemRectMin(), ImGuiCond_None);
      case POPUP_BY_CURSOR:
      default:
        break;
    }
  }

  void PopupHelper::end() { isJustOpened = false; }

  void PopupHelper::close() { isOpen = false; }
}
