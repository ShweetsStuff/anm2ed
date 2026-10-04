#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "types.hpp"

namespace anm2ed::imgui
{
  constexpr auto DRAG_SPEED = 0.25f;
  constexpr auto DRAG_SPEED_FAST = 1.00f;
  constexpr auto STEP = 1.0f;
  constexpr auto STEP_FAST = 5.0f;

  int input_text_callback(ImGuiInputTextCallbackData*);
  bool input_text_string(const char*, std::string*, ImGuiInputTextFlags = 0);
  bool input_text_path(const char*, std::filesystem::path*, ImGuiInputTextFlags = 0);
  bool input_int_range(const char*, int&, int, int, int = STEP, int = STEP_FAST, ImGuiInputTextFlags = 0);
  bool input_int2_range(const char*, glm::ivec2&, glm::ivec2, glm::ivec2, ImGuiInputTextFlags = 0);
  bool input_float_range(const char*, float&, float, float, float = STEP, float = STEP_FAST, const char* = "%.3f",
                         ImGuiInputTextFlags = 0);
  bool input_percent_range(const char*, float&, float, float, float = STEP, float = STEP_FAST);
  types::edit_state::Type drag_float_persistent(const char*, float*, float = DRAG_SPEED, float = {}, float = {},
                                                const char* = "%.3f", ImGuiSliderFlags = 0);
  types::edit_state::Type drag_float2_persistent(const char*, glm::vec2*, float = DRAG_SPEED, float = {}, float = {},
                                                 const char* = "%.3f", ImGuiSliderFlags = 0);
  types::edit_state::Type color_edit3_persistent(const char*, glm::vec3*, ImGuiColorEditFlags = 0);
  types::edit_state::Type color_edit4_persistent(const char*, glm::vec4*, ImGuiColorEditFlags = 0);
  bool radio_button_icon(const char*, int*, int, ImTextureID, ImVec2, ImVec4 = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
  bool combo_id_mapped(const std::string&, int*, const std::vector<int>&, const std::vector<std::string>&);
}
