#pragma once

#include <imgui/imgui.h>

namespace anm2ed::imgui
{
  float row_widget_width_get(int, float = ImGui::GetContentRegionAvail().x);
  ImVec2 widget_size_with_row_get(int, float = ImGui::GetContentRegionAvail().x);
  float footer_height_get(int = 1);
  ImVec2 footer_size_get(int = 1);
  ImVec2 size_without_footer_get(int = 1);
  ImVec2 child_size_get(int = 1);
  ImVec2 icon_size_get();
}
