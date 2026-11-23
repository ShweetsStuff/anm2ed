#pragma once

#include <algorithm>
#include <string>

#include "vec.h"
#include <imgui/imgui.h>

#include "font.h"
#include "localization.h"

constexpr ImVec2 to_imvec2(const Vec2& v) noexcept
{
    return {v.x, v.y};
}

constexpr Vec2 to_vec2(const ImVec2& v) noexcept
{
    return {v.x, v.y};
}

constexpr IVec2 to_ivec2(const ImVec2& v) noexcept
{
    return Vec2(v.x, v.y);
}

constexpr ImVec4 to_imvec4(const Vec4& v) noexcept
{
    return {v.x, v.y, v.z, v.w};
}

template <typename T> constexpr T& dummy_value()
{
    static T value = {};
    return value;
}

template <typename T> constexpr T& dummy_value_negative()
{
    static T value = -1;
    return value;
}

float row_widget_width_get(int, float = ImGui::GetContentRegionAvail().x);
ImVec2 widget_size_with_row_get(int, float = ImGui::GetContentRegionAvail().x);
float footer_height_get(int = 1);
ImVec2 footer_size_get(int = 1);
ImVec2 size_without_footer_get(int = 1);
ImVec2 child_size_get(int = 1);
ImVec2 icon_size_get();
void set_item_tooltip_shortcut(const char*, const std::string& = {});

inline void tooltip_set(StringType type)
{
    ImGui::SetItemTooltip("%s", localize.get(type));
}

// The next window fills the main viewport below `top`.
inline void main_area_next_set(float top)
{
    auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + top));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, std::max(viewport->Size.y - top, 1.0f)));
}

// Whether the mouse is in [min, max).
inline bool is_mouse_in(ImVec2 min, ImVec2 max)
{
    return is_within(to_vec2(ImGui::GetIO().MousePos), to_vec2(min), to_vec2(max));
}

inline void text_bold_draw(ImFont* font, const char* text)
{
    ImGui::PushFont(font, FONT_SIZE);
    ImGui::TextUnformatted(text);
    ImGui::PopFont();
}
