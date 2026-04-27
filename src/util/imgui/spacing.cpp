#include "spacing.hpp"

#include <algorithm>

namespace anm2ed::util::imgui
{
  float row_item_width_get(int itemCount, float width)
  {
    if (itemCount <= 0) return width;
    return (width - (ImGui::GetStyle().ItemSpacing.x * (float)(itemCount - 1))) / (float)itemCount;
  }

  ImVec2 row_item_size_get(int itemCount, float width) { return ImVec2(row_item_width_get(itemCount, width), 0.0f); }

  int footer_row_count_get(int itemCount, int itemsPerRow)
  {
    if (itemCount <= 0 || itemsPerRow <= 0) return 0;
    return (itemCount + itemsPerRow - 1) / itemsPerRow;
  }

  float footer_height_get(int rowCount)
  {
    if (rowCount <= 0) return 0.0f;
    return ImGui::GetTextLineHeightWithSpacing() * rowCount + ImGui::GetStyle().WindowPadding.y +
           ImGui::GetStyle().ItemSpacing.y * (float)rowCount;
  }

  ImVec2 footer_size_get(int rowCount)
  {
    return ImVec2(ImGui::GetContentRegionAvail().x, footer_height_get(rowCount));
  }

  ImVec2 size_without_footer_get(int rowCount)
  {
    return ImVec2(ImGui::GetContentRegionAvail().x,
                  std::max(0.0f, ImGui::GetContentRegionAvail().y - footer_height_get(rowCount)));
  }
}
