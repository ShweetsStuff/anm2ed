#pragma once

#include "vec.h"
#include <imgui/imgui.h>

bool is_drop_after(ImVec2, ImVec2);
void drop_box_draw(ImDrawList*, ImVec2, ImVec2);
void drop_line_draw(ImDrawList*, ImVec2, ImVec2, bool);
void image_premultiplied_draw(ImTextureID, ImVec2);
void image_premultiplied_draw(ImDrawList*, ImTextureID, ImVec2, ImVec2);
void render_checker_background(ImDrawList*, ImVec2, ImVec2, Vec2, float);
