#include "tooltip.hpp"

#include <format>

#include <imgui/imgui.h>

#include "strings.hpp"

namespace anm2ed::imgui
{
  void set_item_tooltip_shortcut(const char* tooltip, const std::string& shortcut)
  {
    ImGui::SetItemTooltip(
        "%s", std::vformat(localize.get(FORMAT_TOOLTIP_SHORTCUT), std::make_format_args(tooltip, shortcut)).c_str());
  }
}
