#include "draw.hpp"

#include <cmath>

namespace anm2ed::imgui
{
  constexpr ImU32 DROP_LINE_COLOR = IM_COL32(255, 255, 0, 255);
  constexpr float DROP_LINE_THICKNESS = 2.0f;

  struct CheckerStart
  {
    float position{};
    long long index{};
  };

  CheckerStart checker_start(float minCoord, float offset, float step)
  {
    float world = minCoord + offset;
    long long index = static_cast<long long>(std::floor(world / step));
    float first = minCoord - (world - static_cast<float>(index) * step);
    return {first, index};
  }

  bool is_drop_after(ImVec2 min, ImVec2 max)
  {
    return ImGui::GetIO().MousePos.y >= (min.y + max.y) * 0.5f;
  }

  void drop_box_draw(ImDrawList* drawList, ImVec2 min, ImVec2 max)
  {
    if (!drawList) return;
    auto offset = DROP_LINE_THICKNESS * 0.5f;
    drawList->AddRect(ImVec2(min.x + offset, min.y + offset), ImVec2(max.x - offset, max.y - offset),
                      DROP_LINE_COLOR, 0.0f, 0, DROP_LINE_THICKNESS);
  }

  void drop_line_draw(ImDrawList* drawList, ImVec2 min, ImVec2 max, bool isAfter)
  {
    if (!drawList) return;
    auto offset = DROP_LINE_THICKNESS * 0.5f;
    auto y = std::floor(isAfter ? max.y - offset : min.y + offset) + 0.5f;
    drawList->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), DROP_LINE_COLOR, DROP_LINE_THICKNESS);
  }

  void render_checker_background(ImDrawList* drawList, ImVec2 min, ImVec2 max, glm::vec2 offset, float step)
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
}
