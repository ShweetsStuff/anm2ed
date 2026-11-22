
#include "ui/panel.h"

#include "vec.h"

#include "localization.h"
#include "tool.h"
#include "types.h"
#include "ui/layout.h"
#include "ui/shortcut.h"

void tools_update(Manager& manager, Settings& settings, Resources& resources, PopupHelper& colorEditPopup)
{
    auto& document = *manager.get();

    if (ImGui::Begin(localize.get(LABEL_TOOLS_WINDOW), &settings.windowIsTools))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

        auto availableWidth = ImGui::GetContentRegionAvail().x;
        auto size = Vec2(ImGui::GetTextLineHeightWithSpacing() * 1.5f);
        auto usedWidth = ImGui::GetStyle().WindowPadding.x;

        auto tool_use = [&](Tool type) {
            switch (type)
            {
                case TOOL_UNDO:
                    if (document.is_able_to_undo())
                        manager.command_push({manager.selected, [](Manager&, Document& document) { document.undo(); }});
                    break;
                case TOOL_REDO:
                    if (document.is_able_to_redo())
                        manager.command_push({manager.selected, [](Manager&, Document& document) { document.redo(); }});
                    break;
                case TOOL_COLOR:
                    colorEditPopup.open();
                    break;
                default:
                    settings.tool = type;
                    break;
            }
        };

        auto iconTint = settings.theme == THEME_LIGHT ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        for (int i = 0; i < TOOL_COUNT; i++)
        {
            auto& info = TOOLS[i];
            auto isSelected = settings.tool == i;

            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

            if (shortcut(manager.chords[info.shortcut], SCOPE_GLOBAL))
                tool_use((Tool)i);

            auto labelText = info.label;
            if (i == TOOL_COLOR)
            {
                size += to_vec2(ImGui::GetStyle().FramePadding) * 2.0f;
                if (ImGui::ColorButton(labelText, to_imvec4(settings.toolColor), ImGuiColorEditFlags_NoTooltip,
                                       to_imvec2(size)))
                    tool_use((Tool)i);
            }
            else
            {
                if (i == TOOL_UNDO)
                    ImGui::BeginDisabled(!document.is_able_to_undo());
                if (i == TOOL_REDO)
                    ImGui::BeginDisabled(!document.is_able_to_redo());
                if (ImGui::ImageButton(labelText, resources.icon_id_get(info.icon), to_imvec2(size), ImVec2(0, 0),
                                       ImVec2(1, 1), ImVec4(0, 0, 0, 0), iconTint))
                    tool_use((Tool)i);
                if (i == TOOL_UNDO)
                    ImGui::EndDisabled();
                if (i == TOOL_REDO)
                    ImGui::EndDisabled();
            }

            auto widthIncrement = ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x;
            usedWidth += widthIncrement;

            if (usedWidth + widthIncrement < availableWidth)
                ImGui::SameLine();
            else
                usedWidth = ImGui::GetStyle().WindowPadding.x;
            set_item_tooltip_shortcut(localize.get(info.tooltip), settings.*SHORTCUT_MEMBERS[info.shortcut]);

            if (isSelected)
                ImGui::PopStyleColor();
        }

        ImGui::PopStyleVar();

        colorEditPopup.trigger();

        if (ImGui::BeginPopup(colorEditPopup.label()))
        {
            ImGui::ColorPicker4(colorEditPopup.label(), values_get(settings.toolColor));
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}
