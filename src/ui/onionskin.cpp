#include "ui/panel.h"

#include "vec.h"

#include "localization.h"
#include "ui/input.h"
#include "ui/layout.h"
#include "ui/shortcut.h"

constexpr auto FRAMES_MAX = 100;

void onionskin_update(Manager& manager, Settings& settings)
{
    auto& isEnabled = settings.onionskinIsEnabled;
    auto& beforeCount = settings.onionskinBeforeCount;
    auto& beforeColor = settings.onionskinBeforeColor;
    auto& afterCount = settings.onionskinAfterCount;
    auto& afterColor = settings.onionskinAfterColor;
    auto& mode = settings.onionskinMode;

    if (ImGui::Begin(localize.get(LABEL_ONIONSKIN_WINDOW), &settings.windowIsOnionskin))
    {
        auto configure_widgets = [&](const char* separator, int& frames, Vec3& color) {
            ImGui::PushID(separator);
            ImGui::SeparatorText(separator);
            input_int_range(localize.get(BASIC_FRAMES), frames, 0, FRAMES_MAX);
            tooltip_set(TOOLTIP_ONIONSKIN_FRAMES);
            ImGui::ColorEdit3(localize.get(BASIC_COLOR), values_get(color));
            tooltip_set(TOOLTIP_ONIONSKIN_COLOR);
            ImGui::PopID();
        };

        ImGui::Checkbox(localize.get(BASIC_ENABLED), &isEnabled);
        set_item_tooltip_shortcut(localize.get(TOOLTIP_ONIONSKIN_ENABLED), settings.shortcutOnionskin);

        configure_widgets(localize.get(BASIC_BEFORE), beforeCount, beforeColor);
        configure_widgets(localize.get(BASIC_AFTER), afterCount, afterColor);
        ImGui::SeparatorText(localize.get(BASIC_MODE));
        ImGui::RadioButton(localize.get(BASIC_TIME), &mode, (int)OnionskinMode::TIME);
        tooltip_set(TOOLTIP_ONIONSKIN_TIME);
        ImGui::SameLine();
        ImGui::RadioButton(localize.get(BASIC_INDEX), &mode, (int)OnionskinMode::INDEX);
        tooltip_set(TOOLTIP_ONIONSKIN_INDEX);

        ImGui::SeparatorText(localize.get(LABEL_OPTIONS));
        ImGui::Checkbox(localize.get(LABEL_PIVOT_PATH), &settings.onionskinIsPivotPath);
        tooltip_set(TOOLTIP_PIVOT_PATH);
        ImGui::SameLine();
        ImGui::Checkbox(localize.get(LABEL_ONLY_SELECTED_ITEM), &settings.onionskinIsOnlySelected);
        tooltip_set(TOOLTIP_ONLY_SELECTED_ITEM);
    }
    ImGui::End();

    if (shortcut(manager.chords[SHORTCUT_ONIONSKIN], SCOPE_GLOBAL))
        isEnabled = !isEnabled;
}
