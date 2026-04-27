#pragma once

#include <imgui/imgui.h>

namespace anm2ed::util::imgui
{
  float row_item_width_get(int, float = ImGui::GetContentRegionAvail().x);
  ImVec2 row_item_size_get(int, float = ImGui::GetContentRegionAvail().x);
  int footer_row_count_get(int, int);
  float footer_height_get(int);
  ImVec2 footer_size_get(int);
  ImVec2 size_without_footer_get(int);
}
