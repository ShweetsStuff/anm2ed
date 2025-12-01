#include "onionskin.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui_.h"
#include "strings.h"

using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::imgui
{
  constexpr auto FRAMES_MAX = 100;

  void Onionskin::update(Manager& manager, Settings& settings)
  {
    auto& isEnabled = settings.onionskinIsEnabled;
    auto& beforeCount = settings.onionskinBeforeCount;
    auto& beforeColor = settings.onionskinBeforeColor;
    auto& afterCount = settings.onionskinAfterCount;
    auto& afterColor = settings.onionskinAfterColor;
    auto& mode = settings.onionskinMode;

    if (ImGui::Begin(localize.get(LABEL_ONIONSKIN_WINDOW), &settings.windowIsOnionskin))
    {
      auto configure_widgets = [&](const char* separator, int& frames, vec3& color)
      {
        ImGui::PushID(separator);
        ImGui::SeparatorText(separator);
        input_int_range(localize.get(BASIC_FRAMES), frames, 0, FRAMES_MAX);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ONIONSKIN_FRAMES));
        ImGui::ColorEdit3(localize.get(BASIC_COLOR), value_ptr(color));
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ONIONSKIN_COLOR));
        ImGui::PopID();
      };

      ImGui::Checkbox(localize.get(BASIC_ENABLED), &isEnabled);
      set_item_tooltip_shortcut(localize.get(TOOLTIP_ONIONSKIN_ENABLED), settings.shortcutOnionskin);

      configure_widgets(localize.get(BASIC_BEFORE), beforeCount, beforeColor);
      configure_widgets(localize.get(BASIC_AFTER), afterCount, afterColor);
      ImGui::SeparatorText(localize.get(BASIC_MODE));
      ImGui::RadioButton(localize.get(BASIC_TIME), &mode, (int)OnionskinMode::TIME);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ONIONSKIN_TIME));
      ImGui::SameLine();
      ImGui::RadioButton(localize.get(BASIC_INDEX), &mode, (int)OnionskinMode::INDEX);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ONIONSKIN_INDEX));
    }
    ImGui::End();

    if (shortcut(manager.chords[SHORTCUT_ONIONSKIN], shortcut::GLOBAL)) isEnabled = !isEnabled;
  }

}
