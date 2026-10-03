#pragma once

#include <string>

#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace anm2ed::types
{
  constexpr ImVec2 to_imvec2(const glm::vec2& v) noexcept { return {v.x, v.y}; }
  constexpr glm::vec2 to_vec2(const ImVec2& v) noexcept { return {v.x, v.y}; }
  constexpr glm::ivec2 to_ivec2(const ImVec2& v) noexcept { return {v.x, v.y}; }
  constexpr ImVec4 to_imvec4(const glm::vec4& v) noexcept { return {v.x, v.y, v.z, v.w}; }
  constexpr glm::vec4 to_vec4(const ImVec4& v) noexcept { return {v.x, v.y, v.z, v.w}; }

  template <typename T> constexpr T& dummy_value()
  {
    static T value{};
    return value;
  }

  template <typename T> constexpr T& dummy_value_negative()
  {
    static T value{-1};
    return value;
  }
}

namespace anm2ed::imgui
{
  float row_widget_width_get(int, float = ImGui::GetContentRegionAvail().x);
  ImVec2 widget_size_with_row_get(int, float = ImGui::GetContentRegionAvail().x);
  float footer_height_get(int = 1);
  ImVec2 footer_size_get(int = 1);
  ImVec2 size_without_footer_get(int = 1);
  ImVec2 child_size_get(int = 1);
  ImVec2 icon_size_get();
  void set_item_tooltip_shortcut(const char*, const std::string& = {});
}
