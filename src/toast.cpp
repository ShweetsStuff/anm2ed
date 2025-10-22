#include "toast.h"

#include "log.h"
#include <imgui/imgui.h>

#include "types.h"

using namespace anm2ed::log;
using namespace anm2ed::types;

namespace anm2ed::toast
{
  constexpr auto LIFETIME = 3.0f;

  Toast::Toast(const std::string& message)
  {
    this->message = message;
    lifetime = LIFETIME;
  }

  void Toasts::update()
  {
    ImGuiIO& io = ImGui::GetIO();

    auto borderColor = ImGui::GetStyleColorVec4(ImGuiCol_Border);
    auto textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    auto position = to_vec2(io.DisplaySize) - to_vec2(ImGui::GetStyle().ItemSpacing);

    for (int i = (int)toasts.size() - 1; i >= 0; --i)
    {
      Toast& toast = toasts[i];

      toast.lifetime -= ImGui::GetIO().DeltaTime;

      if (toast.lifetime <= 0.0f)
      {
        toasts.erase(toasts.begin() + i);
        i--;
        continue;
      }

      auto alpha = toast.lifetime / LIFETIME;
      borderColor.w = alpha;
      textColor.w = alpha;

      ImGui::SetNextWindowPos(to_imvec2(position), ImGuiCond_None, {1.0f, 1.0f});
      ImGui::SetNextWindowSize(ImVec2(0, ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y));
      ImGui::PushStyleColor(ImGuiCol_Text, textColor);
      ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
      ImGui::SetNextWindowBgAlpha(alpha);

      if (ImGui::Begin(std::format("##Toast #{}", i).c_str(), nullptr,
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                           ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      {
        ImGui::TextUnformatted(toast.message.c_str());
        position.y -= ImGui::GetWindowSize().y + ImGui::GetStyle().ItemSpacing.y;
      }
      ImGui::End();
      ImGui::PopStyleColor(2);
    }
  }

  void Toasts::add(const std::string& message)
  {
    toasts.emplace_back(Toast(message));
    logger.info(message);
  }

  void Toasts::add_error(const std::string& message)
  {
    toasts.emplace_back(Toast(message));
    logger.error(message);
  }

  Toasts toasts;
}
