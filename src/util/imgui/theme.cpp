#include "theme.hpp"

namespace anm2ed::imgui
{
  constexpr ImVec4 COLOR_LIGHT_BUTTON{0.98f, 0.98f, 0.98f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TITLE_BG{0.78f, 0.78f, 0.78f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TITLE_BG_ACTIVE{0.64f, 0.64f, 0.64f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TITLE_BG_COLLAPSED{0.74f, 0.74f, 0.74f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TABLE_HEADER{0.78f, 0.78f, 0.78f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB{0.74f, 0.74f, 0.74f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_HOVERED{0.82f, 0.82f, 0.82f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_SELECTED{0.92f, 0.92f, 0.92f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_DIMMED{0.70f, 0.70f, 0.70f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_DIMMED_SELECTED{0.86f, 0.86f, 0.86f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_OVERLINE{0.55f, 0.55f, 0.55f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_TAB_DIMMED_OVERLINE{0.50f, 0.50f, 0.50f, 1.0f};
  constexpr ImVec4 COLOR_LIGHT_CHECK_MARK{0.0f, 0.0f, 0.0f, 1.0f};
  constexpr auto FRAME_BORDER_SIZE = 1.0f;

  void theme_set(types::theme::Type theme)
  {
    switch (theme)
    {
      case types::theme::LIGHT:
        ImGui::StyleColorsLight();
        break;
      case types::theme::DARK:
      default:
        ImGui::StyleColorsDark();
        break;
      case types::theme::CLASSIC:
        ImGui::StyleColorsClassic();
        break;
    }
    auto& style = ImGui::GetStyle();
    style.FrameBorderSize = FRAME_BORDER_SIZE;

    if (theme == types::theme::LIGHT)
    {
      auto& colors = style.Colors;
      colors[ImGuiCol_Button] = COLOR_LIGHT_BUTTON;
      colors[ImGuiCol_TitleBg] = COLOR_LIGHT_TITLE_BG;
      colors[ImGuiCol_TitleBgActive] = COLOR_LIGHT_TITLE_BG_ACTIVE;
      colors[ImGuiCol_TitleBgCollapsed] = COLOR_LIGHT_TITLE_BG_COLLAPSED;
      colors[ImGuiCol_TableHeaderBg] = COLOR_LIGHT_TABLE_HEADER;
      colors[ImGuiCol_Tab] = COLOR_LIGHT_TAB;
      colors[ImGuiCol_TabHovered] = COLOR_LIGHT_TAB_HOVERED;
      colors[ImGuiCol_TabSelected] = COLOR_LIGHT_TAB_SELECTED;
      colors[ImGuiCol_TabSelectedOverline] = COLOR_LIGHT_TAB_OVERLINE;
      colors[ImGuiCol_TabDimmed] = COLOR_LIGHT_TAB_DIMMED;
      colors[ImGuiCol_TabDimmedSelected] = COLOR_LIGHT_TAB_DIMMED_SELECTED;
      colors[ImGuiCol_TabDimmedSelectedOverline] = COLOR_LIGHT_TAB_DIMMED_OVERLINE;
      colors[ImGuiCol_CheckMark] = COLOR_LIGHT_CHECK_MARK;
    }
  }
}
