#include "ui/popup.h"

#include "ui/layout.h"

#include <imgui/imgui.h>

#include "types.h"

int PopupHelper::nextId = 1;

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

PopupHelper::PopupHelper(StringType labelId, PopupType type, PopupPosition position)
{
    this->labelId = labelId;
    this->type = type;
    this->position = position;
    id = nextId++;
}

const char* PopupHelper::label() const
{
    labelText = localize.get(labelId);
    labelText += "##Popup";
    labelText += std::to_string(id);
    return labelText.c_str();
}

void PopupHelper::open()
{
    isOpen = true;
    isTriggered = true;
    isJustOpened = true;
}

bool PopupHelper::is_open()
{
    return isOpen;
}

void PopupHelper::trigger()
{
    if (isTriggered)
        ImGui::OpenPopup(label());
    isTriggered = false;

    auto viewport = ImGui::GetMainViewport();

    switch (position)
    {
        case POPUP_CENTER:
            ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_None, to_imvec2(Vec2(0.5f)));
            if (POPUP_IS_HEIGHT_SET[type])
                ImGui::SetNextWindowSize(to_imvec2(to_vec2(viewport->Size) * POPUP_MULTIPLIERS[type]));
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

void PopupHelper::end()
{
    isJustOpened = false;
}

void PopupHelper::close()
{
    isOpen = false;
}
