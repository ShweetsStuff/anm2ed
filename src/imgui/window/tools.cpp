
#include "tools.h"

#include <glm/gtc/type_ptr.hpp>

#include "tool.h"
#include "types.h"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::imgui
{
  void Tools::update(Manager& manager, Settings& settings, Resources& resources)
  {
    auto& document = *manager.get();

    if (ImGui::Begin("Tools", &settings.windowIsTools))
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

      auto availableWidth = ImGui::GetContentRegionAvail().x;
      auto size = vec2(ImGui::GetTextLineHeightWithSpacing() * 1.5f);
      auto usedWidth = ImGui::GetStyle().WindowPadding.x;

      auto tool_use = [&](tool::Type type)
      {
        switch (type)
        {
          case tool::UNDO:
            if (document.is_able_to_undo()) document.undo();
            break;
          case tool::REDO:
            if (document.is_able_to_redo()) document.redo();
            break;
          case tool::COLOR:
            colorEditPopup.open();
            break;
          default:
            settings.tool = type;
            break;
        }
      };

      auto iconTint = settings.theme == theme::LIGHT ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

      for (int i = 0; i < tool::COUNT; i++)
      {
        auto& info = tool::INFO[i];
        auto isSelected = settings.tool == i;

        if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

        if (shortcut(manager.chords[info.shortcut], shortcut::GLOBAL)) tool_use((tool::Type)i);

        if (i == tool::COLOR)
        {
          size += to_vec2(ImGui::GetStyle().FramePadding) * 2.0f;
          if (ImGui::ColorButton(info.label, to_imvec4(settings.toolColor), ImGuiColorEditFlags_NoTooltip,
                                 to_imvec2(size)))
            tool_use((tool::Type)i);

          colorEditPosition = ImGui::GetCursorScreenPos();
        }
        else
        {
          if (i == tool::UNDO) ImGui::BeginDisabled(!document.is_able_to_undo());
          if (i == tool::REDO) ImGui::BeginDisabled(!document.is_able_to_redo());
          if (ImGui::ImageButton(info.label, resources.icons[info.icon].id, to_imvec2(size), ImVec2(0, 0), ImVec2(1, 1),
                                 ImVec4(0, 0, 0, 0), iconTint))
            tool_use((tool::Type)i);
          if (i == tool::UNDO) ImGui::EndDisabled();
          if (i == tool::REDO) ImGui::EndDisabled();
        }

        auto widthIncrement = ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x;
        usedWidth += widthIncrement;

        if (usedWidth + widthIncrement < availableWidth)
          ImGui::SameLine();
        else
          usedWidth = ImGui::GetStyle().WindowPadding.x;
        set_item_tooltip_shortcut(info.tooltip, settings.*SHORTCUT_MEMBERS[info.shortcut]);

        if (isSelected) ImGui::PopStyleColor();
      }

      ImGui::PopStyleVar();

      colorEditPopup.trigger();

      if (ImGui::BeginPopup(colorEditPopup.label))
      {
        ImGui::ColorPicker4(colorEditPopup.label, value_ptr(settings.toolColor));
        ImGui::EndPopup();
      }
    }
    ImGui::End();
  }
}
