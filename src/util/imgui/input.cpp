#include "input.hpp"

#include <algorithm>
#include <cmath>
#include <climits>

#include <glm/gtc/type_ptr.hpp>

#include "path.hpp"

using namespace anm2ed::types;
using namespace anm2ed::util;
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

  bool input_text_path(const char* label, std::filesystem::path* path, ImGuiInputTextFlags flags)
  {
    if (!path) return false;

    auto pathUtf8 = path::to_utf8(*path);
    auto edited = input_text_string(label, &pathUtf8, flags);
    if (edited) *path = path::from_utf8(pathUtf8);

    return edited;
  }

  bool combo_negative_one_indexed(const std::string& label, int* index, std::vector<const char*>& strings)
  {
    *index += 1;
    bool isActivated = ImGui::Combo(label.c_str(), index, strings.data(), (int)strings.size());
    *index -= 1;

    return isActivated;
  }

  bool combo_id_mapped(const std::string& label, int* id, const std::vector<int>& ids, std::vector<const char*>& labels)
  {
    if (!id) return false;

    int index = -1;
    if (!ids.empty())
    {
      auto it = std::find(ids.begin(), ids.end(), *id);
      if (it != ids.end()) index = (int)std::distance(ids.begin(), it);
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

  edit::Type drag_int_persistent(const char* label, int* value, float speed, int min, int max, const char* format,
                                 ImGuiSliderFlags flags)
  {
    static bool isEditing{};
    static int start{INT_MAX};
    auto persistent = value ? *value : 0;

    ImGui::DragInt(label, &persistent, speed, min, max, format, flags);
    if (!value) return edit::NONE;
    if (ImGui::IsItemActivated() && persistent != start)
    {
      isEditing = true;
      start = *value;
      return edit::START;
    }
    else if (ImGui::IsItemDeactivatedAfterEdit())
    {
      isEditing = false;
      *value = persistent;
      start = INT_MAX;
      return edit::END;
    }
    else if (isEditing)
    {
      *value = persistent;
      return edit::DURING;
    }

    return edit::NONE;
  }

  edit::Type drag_float_persistent(const char* label, float* value, float speed, float min, float max,
                                   const char* format, ImGuiSliderFlags flags)
  {
    static bool isEditing{};
    static float start{NAN};
    auto persistent = value ? *value : 0;

    ImGui::DragFloat(label, &persistent, speed, min, max, format, flags);
    if (!value) return edit::NONE;
    if (ImGui::IsItemActivated() && persistent != start)
    {
      isEditing = true;
      start = *value;
      return edit::START;
    }
    else if (ImGui::IsItemDeactivatedAfterEdit())
    {
      isEditing = false;
      *value = persistent;
      start = NAN;
      return edit::END;
    }
    else if (isEditing)
    {
      *value = persistent;
      return edit::DURING;
    }

    return edit::NONE;
  }

  edit::Type drag_float2_persistent(const char* label, vec2* value, float speed, float min, float max,
                                    const char* format, ImGuiSliderFlags flags)
  {
    static bool isEditing{};
    static vec2 start{NAN};
    auto persistent = value ? *value : vec2();

    ImGui::DragFloat2(label, value_ptr(persistent), speed, min, max, format, flags);
    if (!value) return edit::NONE;
    if (ImGui::IsItemActivated() && persistent != start)
    {
      isEditing = true;
      start = *value;
      return edit::START;
    }
    else if (ImGui::IsItemDeactivatedAfterEdit())
    {
      isEditing = false;
      *value = persistent;
      start = vec2{NAN};
      return edit::END;
    }
    else if (isEditing)
    {
      *value = persistent;
      return edit::DURING;
    }

    return edit::NONE;
  }

  edit::Type color_edit3_persistent(const char* label, vec3* value, ImGuiColorEditFlags flags)
  {
    static bool isEditing{};
    static vec3 start{NAN};
    auto persistent = value ? *value : vec4();

    ImGui::ColorEdit3(label, value_ptr(persistent), flags);
    if (!value) return edit::NONE;
    if (ImGui::IsItemActivated() && persistent != start)
    {
      isEditing = true;
      start = *value;
      return edit::START;
    }
    else if (ImGui::IsItemDeactivatedAfterEdit())
    {
      isEditing = false;
      *value = persistent;
      start = vec4{NAN};
      return edit::END;
    }
    else if (isEditing)
    {
      *value = persistent;
      return edit::DURING;
    }

    return edit::NONE;
  }

  edit::Type color_edit4_persistent(const char* label, vec4* value, ImGuiColorEditFlags flags)
  {
    static bool isEditing{};
    static vec4 start{NAN};
    auto persistent = value ? *value : vec4();

    ImGui::ColorEdit4(label, value_ptr(persistent), flags);
    if (!value) return edit::NONE;
    if (ImGui::IsItemActivated() && persistent != start)
    {
      isEditing = true;
      start = *value;
      return edit::START;
    }
    else if (ImGui::IsItemDeactivatedAfterEdit())
    {
      isEditing = false;
      *value = persistent;
      start = vec4{NAN};
      return edit::END;
    }
    else if (isEditing)
    {
      *value = persistent;
      return edit::DURING;
    }

    return edit::NONE;
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
}
