#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "constants.hpp"
#include "types.hpp"

namespace anm2ed::imgui
{
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
  bool combo_id_mapped(const std::string&, int*, const std::vector<int>&, std::vector<const char*>&);
}
