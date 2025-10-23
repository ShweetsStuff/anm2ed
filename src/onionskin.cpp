#include "onionskin.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"

using namespace anm2ed::settings;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::onionskin
{
  void Onionskin::update(Settings& settings)
  {
    if (ImGui::Begin("Onionskin", &settings.windowIsOnionskin))
    {
      auto order_configure = [&](const std::string& separator, int& frames, vec3& color)
      {
        ImGui::PushID(separator.c_str());
        ImGui::SeparatorText(separator.c_str());
        ImGui::InputInt("Frames", &frames, 1, 5);
        frames = glm::clamp(frames, 0, 100);
        ImGui::ColorEdit3("Color", value_ptr(color));
        ImGui::PopID();
      };

      imgui::shortcut(settings.shortcutOnionskin);
      ImGui::Checkbox("Enabled", &settings.onionskinIsEnabled);

      order_configure("Before", settings.onionskinBeforeCount, settings.onionskinBeforeColor);
      order_configure("After", settings.onionskinAfterCount, settings.onionskinAfterColor);

      ImGui::Text("Order");
      ImGui::SameLine();
      ImGui::RadioButton("Before", &settings.onionskinDrawOrder, BELOW);
      ImGui::SameLine();
      ImGui::RadioButton("After", &settings.onionskinDrawOrder, ABOVE);
    }

    if (imgui::shortcut(settings.shortcutOnionskin), shortcut::GLOBAL)
      settings.onionskinIsEnabled = !settings.onionskinIsEnabled;

    ImGui::End();
  }
}
