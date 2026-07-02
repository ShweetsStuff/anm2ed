#pragma once

#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace anm2ed::imgui
{
  bool is_drop_after(ImVec2, ImVec2);
  void drop_box_draw(ImDrawList*, ImVec2, ImVec2);
  void drop_line_draw(ImDrawList*, ImVec2, ImVec2, bool);
  void image_premultiplied_draw(ImTextureID, ImVec2);
  void image_premultiplied_draw(ImDrawList*, ImTextureID, ImVec2, ImVec2);
  void render_checker_background(ImDrawList*, ImVec2, ImVec2, glm::vec2, float);
}
