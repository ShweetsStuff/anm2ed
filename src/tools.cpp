
#include "tools.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "tool.h"
#include "types.h"

using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::tools
{
  constexpr auto COLOR_EDIT_LABEL = "##Color Edit";

  void Tools::update(Settings& settings, Resources& resources)
  {
    if (ImGui::Begin("Tools", &settings.windowIsTools))
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

      auto availableWidth = ImGui::GetContentRegionAvail().x;
      auto size = vec2(ImGui::GetTextLineHeightWithSpacing() * 1.5f);
      auto usedWidth = ImGui::GetStyle().WindowPadding.x;

      auto tool_switch = [&](tool::Type type)
      {
        switch (type)
        {
          case tool::UNDO:
            break;
          case tool::REDO:
            break;
          case tool::COLOR:
            if (ImGui::IsPopupOpen(COLOR_EDIT_LABEL))
              ImGui::CloseCurrentPopup();
            else
              isOpenColorEdit = true;
            break;
          default:
            settings.tool = type;
            break;
        }
      };

      for (int i = 0; i < tool::COUNT; i++)
      {
        auto& info = tool::INFO[i];
        auto isSelected = settings.tool == i;

        if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

        auto member = SHORTCUT_MEMBERS[info.shortcut];

        if (imgui::shortcut(settings.*member, false, true, i == tool::COLOR ? false : true)) tool_switch((tool::Type)i);

        if (i == tool::COLOR)
        {
          size += to_vec2(ImGui::GetStyle().FramePadding) * 2.0f;
          if (ImGui::ColorButton(info.label, to_imvec4(settings.toolColor), ImGuiColorEditFlags_NoTooltip,
                                 to_imvec2(size)))
            tool_switch((tool::Type)i);

          colorEditPosition = ImGui::GetCursorScreenPos();
        }
        else
        {
          if (ImGui::ImageButton(info.label, resources.icons[info.icon].id, to_imvec2(size)))
            tool_switch((tool::Type)i);
        }

        auto widthIncrement = ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x;
        usedWidth += widthIncrement;

        if (usedWidth + widthIncrement < availableWidth)
          ImGui::SameLine();
        else
          usedWidth = ImGui::GetStyle().WindowPadding.x;

        imgui::set_item_tooltip_shortcut(info.tooltip, settings.*SHORTCUT_MEMBERS[info.shortcut]);

        if (isSelected) ImGui::PopStyleColor();
      }

      ImGui::PopStyleVar();

      if (isOpenColorEdit)
      {
        ImGui::OpenPopup(COLOR_EDIT_LABEL);
        isOpenColorEdit = false;
      }

      ImGui::SetNextWindowPos(colorEditPosition, ImGuiCond_None);

      if (ImGui::BeginPopup(COLOR_EDIT_LABEL))
      {
        ImGui::ColorPicker4(COLOR_EDIT_LABEL, value_ptr(settings.toolColor));
        ImGui::EndPopup();
      }
    }
    ImGui::End();
  }
}
