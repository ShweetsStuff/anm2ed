#include "ui/shortcut.h"

#include <sstream>
#include <utility>
#include <vector>

#include <imgui/imgui_internal.h>

// Names shortcuts are saved with where they differ from ImGui's. ImGui's own names still read back.
constexpr std::pair<ImGuiKey, const char*> KEY_NAMES[] = {{ImGuiKey_Keypad0, "Num0"},
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
                                                          {ImGuiKey_UpArrow, "Up"},
                                                          {ImGuiKey_DownArrow, "Down"},
                                                          {ImGuiKey_LeftArrow, "Left"},
                                                          {ImGuiKey_RightArrow, "Right"}};

constexpr std::pair<ImGuiKey, const char*> MOD_NAMES[] = {
    {ImGuiMod_Ctrl, "Ctrl"}, {ImGuiMod_Shift, "Shift"}, {ImGuiMod_Alt, "Alt"}, {ImGuiMod_Super, "Super"}};

bool is_shortcut_key(ImGuiKey key)
{
    return key >= ImGuiKey_Tab && key <= ImGuiKey_KeypadEqual && (key < ImGuiKey_LeftCtrl || key > ImGuiKey_RightSuper);
}

static const char* key_name_get(ImGuiKey key)
{
    for (auto [named, name] : KEY_NAMES)
        if (named == key)
            return name;
    return ImGui::GetKeyName(key);
}

std::string chord_to_string(ImGuiKeyChord chord)
{
    std::string result;
    for (auto [mod, name] : MOD_NAMES)
        if (chord & mod)
            result += std::string(name) + "+";
    if (auto key = (ImGuiKey)(chord & ~ImGuiMod_Mask_); key != ImGuiKey_None)
        result += key_name_get(key);
    if (!result.empty() && result.back() == '+')
        result.pop_back();
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
        for (auto [mod, name] : MOD_NAMES)
            if (token == name)
                chord |= mod;
        for (int key = ImGuiKey_Tab; baseKey == ImGuiKey_None && key <= ImGuiKey_KeypadEqual; ++key)
            if (is_shortcut_key((ImGuiKey)key) &&
                (token == key_name_get((ImGuiKey)key) || token == ImGui::GetKeyName((ImGuiKey)key)))
                baseKey = (ImGuiKey)key;
    }
    return chord | baseKey;
}

bool shortcut(ImGuiKeyChord chord, ShortcutScope type)
{
    if (chord == ImGuiKey_None)
        return false;

    if (ImGui::GetTopMostPopupModal() != nullptr && (type == SCOPE_GLOBAL || type == SCOPE_GLOBAL_SET))
        return false;

    int flags =
        type == SCOPE_GLOBAL || type == SCOPE_GLOBAL_SET ? ImGuiInputFlags_RouteGlobal : ImGuiInputFlags_RouteFocused;
    flags |= ImGuiInputFlags_Repeat;

    if (type == SCOPE_GLOBAL_SET || type == SCOPE_FOCUSED_SET)
    {
        ImGui::SetNextItemShortcut(chord, flags);
        return false;
    }

    return ImGui::Shortcut(chord, flags);
}
