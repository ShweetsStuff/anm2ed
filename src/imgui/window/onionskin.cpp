#include "onionskin.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui_.h"

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

    if (ImGui::Begin("Onionskin", &settings.windowIsOnionskin))
    {
      auto configure_widgets = [&](const char* separator, int& frames, vec3& color)
      {
        ImGui::PushID(separator);
        ImGui::SeparatorText(separator);
        input_int_range("Frames", frames, 0, FRAMES_MAX);
        ImGui::SetItemTooltip("Change the amount of frames this onionskin will use.");
        ImGui::ColorEdit3("Color", value_ptr(color));
        ImGui::SetItemTooltip("Change the color of the frames this onionskin will use.");
        ImGui::PopID();
      };

      ImGui::Checkbox("Enabled", &isEnabled);
      set_item_tooltip_shortcut("Toggle onionskinning.", settings.shortcutOnionskin);

      configure_widgets("Before", beforeCount, beforeColor);
      configure_widgets("After", afterCount, afterColor);
      ImGui::SeparatorText("Mode");
      ImGui::RadioButton("Time", &mode, (int)OnionskinMode::TIME);
      ImGui::SetItemTooltip("The onionskinned frames will be based on frame time.");
      ImGui::SameLine();
      ImGui::RadioButton("Index", &mode, (int)OnionskinMode::INDEX);
      ImGui::SetItemTooltip("The onionskinned frames will be based on frame index.");
    }
    ImGui::End();

    if (shortcut(manager.chords[SHORTCUT_ONIONSKIN], shortcut::GLOBAL)) isEnabled = !isEnabled;
  }

}
