#include "popup.hpp"

#include <imgui/imgui.h>

#include "types.hpp"

namespace anm2ed::imgui
{
#define POPUP_LIST                                                                                                     \
  X(POPUP_SMALL, 0.25f, true)                                                                                          \
  X(POPUP_NORMAL, 0.5f, true)                                                                                          \
  X(POPUP_TO_CONTENT, 0.0f, true)                                                                                      \
  X(POPUP_SMALL_NO_HEIGHT, 0.25f, false)                                                                               \
  X(POPUP_NORMAL_NO_HEIGHT, 0.5f, false)

  constexpr float POPUP_MULTIPLIERS[] = {
#define X(name, multiplier, isHeightSet) multiplier,
      POPUP_LIST
#undef X
  };

  constexpr bool POPUP_IS_HEIGHT_SET[] = {
#define X(name, multiplier, isHeightSet) isHeightSet,
      POPUP_LIST
#undef X
  };

#undef POPUP_LIST

  PopupHelper::PopupHelper(StringType labelId, PopupType type, PopupPosition position)
  {
    this->labelId = labelId;
    this->type = type;
    this->position = position;
  }

  void PopupHelper::open()
  {
    isOpen = true;
    isTriggered = true;
    isJustOpened = true;
  }

  bool PopupHelper::is_open() { return isOpen; }

  void PopupHelper::trigger()
  {
    if (isTriggered) ImGui::OpenPopup(localize.get(labelId));
    isTriggered = false;

    auto viewport = ImGui::GetMainViewport();

    switch (position)
    {
      case POPUP_CENTER:
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_None, types::to_imvec2(glm::vec2(0.5f)));
        if (POPUP_IS_HEIGHT_SET[type])
          ImGui::SetNextWindowSize(types::to_imvec2(types::to_vec2(viewport->Size) * POPUP_MULTIPLIERS[type]));
        else
          ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * POPUP_MULTIPLIERS[type], 0));
        break;
      case POPUP_BY_ITEM:
        ImGui::SetNextWindowPos(ImGui::GetItemRectMin(), ImGuiCond_None);
      case POPUP_BY_CURSOR:
      default:
        break;
    }
  }

  void PopupHelper::end() { isJustOpened = false; }

  void PopupHelper::close() { isOpen = false; }
}
