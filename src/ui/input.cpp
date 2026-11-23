#include "ui/input.h"
#include "ui/layout.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <numeric>

#include "vec.h"

#include "path.h"

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

bool input_text_path(const char* label, std::filesystem::path* path, ImGuiInputTextFlags flags)
{
    if (!path)
        return false;

    auto pathUtf8 = path_to_utf8(*path);
    auto edited = input_text_string(label, &pathUtf8, flags);
    if (edited)
        *path = path_from_utf8(pathUtf8);

    return edited;
}

// A combo over `values` shown as `labels`.
bool combo_string_mapped(const std::string& label, std::string* value, const std::vector<std::string>& values,
                         const std::vector<std::string>& labels)
{
    std::vector<int> ids(values.size());
    std::iota(ids.begin(), ids.end(), 0);
    auto index = (int)std::distance(values.begin(), std::ranges::find(values, *value));
    if (!combo_id_mapped(label, &index, ids, labels) || index < 0)
        return false;
    *value = values[index];
    return true;
}

bool combo_id_mapped(const std::string& label, int* id, const std::vector<int>& ids,
                     const std::vector<std::string>& labelStrings)
{
    std::vector<const char*> labels;
    for (auto& labelString : labelStrings)
        labels.push_back(labelString.c_str());
    if (!id)
        return false;

    int index = -1;
    if (!ids.empty())
    {
        auto it = std::find(ids.begin(), ids.end(), *id);
        if (it != ids.end())
            index = (int)std::distance(ids.begin(), it);
    }

    bool isActivated = ImGui::Combo(label.c_str(), &index, labels.data(), (int)labels.size());
    if (isActivated)
    {
        if (index >= 0 && index < (int)ids.size())
            *id = ids[index];
        else
            *id = -1;
    }

    return isActivated;
}

// Follows an edit made on a copy of `value`: it starts on activation (when the copy differs), ends once deactivated
// after an edit, and is under way in between, writing the copy back. Each value type keeps its own state.
template <class T> static EditState persistent_state_get(T* value, T persistent)
{
    static bool isEditing = false;
    static T start = T(NAN);
    if (!value)
        return EDIT_STATE_NONE;
    if (ImGui::IsItemActivated() && persistent != start)
    {
        isEditing = true;
        start = *value;
        return EDIT_STATE_START;
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        isEditing = false;
        *value = persistent;
        start = T(NAN);
        return EDIT_STATE_END;
    }
    if (!isEditing)
        return EDIT_STATE_NONE;
    *value = persistent;
    return EDIT_STATE_DURING;
}

EditState drag_float_persistent(const char* label, float* value, float speed, float min, float max, const char* format,
                                ImGuiSliderFlags flags)
{
    auto persistent = value ? *value : 0;
    ImGui::DragFloat(label, &persistent, speed, min, max, format, flags);
    return persistent_state_get(value, persistent);
}

EditState drag_float2_persistent(const char* label, Vec2* value, float speed, float min, float max, const char* format,
                                 ImGuiSliderFlags flags)
{
    auto persistent = value ? *value : Vec2();
    ImGui::DragFloat2(label, values_get(persistent), speed, min, max, format, flags);
    return persistent_state_get(value, persistent);
}

EditState color_edit4_persistent(const char* label, Vec4* value, ImGuiColorEditFlags flags)
{
    auto persistent = value ? *value : Vec4();
    ImGui::ColorEdit4(label, values_get(persistent), flags);
    return persistent_state_get(value, persistent);
}

bool radio_button_icon(const char* label, int* value, int buttonValue, ImTextureID texture, ImVec2 size, ImVec4 tint)
{
    bool isSelected = value && *value == buttonValue;
    if (isSelected)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

    bool isActivated = ImGui::ImageButton(label, texture, size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tint);

    if (isSelected)
        ImGui::PopStyleColor();
    if (isActivated && value)
        *value = buttonValue;
    return isActivated;
}

bool input_int_range(const char* label, int& value, int min, int max, int step, int stepFast, ImGuiInputTextFlags flags)
{
    auto isActivated = ImGui::InputInt(label, &value, step, stepFast, flags);
    value = vec_clamp(value, min, max);
    return isActivated;
}

bool input_int2_range(const char* label, IVec2& value, IVec2 min, IVec2 max, ImGuiInputTextFlags flags)
{
    auto isActivated = ImGui::InputInt2(label, values_get(value), flags);
    value = vec_clamp(value, min, max);
    return isActivated;
}

bool input_float_range(const char* label, float& value, float min, float max, float step, float stepFast,
                       const char* format, ImGuiInputTextFlags flags)
{
    auto isActivated = ImGui::InputFloat(label, &value, step, stepFast, format, flags);
    value = vec_clamp(value, min, max);
    return isActivated;
}

// A localized checkbox or radio button, with its tooltip.
bool checkbox_draw(StringType label, bool& value, StringType tooltip)
{
    auto isChanged = ImGui::Checkbox(localize.get(label), &value);
    if (tooltip != STRING_UNDEFINED)
        tooltip_set(tooltip);
    return isChanged;
}

bool radio_draw(StringType label, int& value, int buttonValue, StringType tooltip)
{
    auto isChanged = ImGui::RadioButton(localize.get(label), &value, buttonValue);
    if (tooltip != STRING_UNDEFINED)
        tooltip_set(tooltip);
    return isChanged;
}
