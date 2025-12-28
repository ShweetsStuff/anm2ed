#include "change_all_frame_properties.h"

#include <string>

#include "math_.h"

using namespace anm2ed::util::math;
using namespace glm;

namespace anm2ed::imgui::wizard
{
  void ChangeAllFrameProperties::update(Document& document, Settings& settings)
  {
    isChanged = false;

    auto& frames = document.frames.selection;
    auto& isCropX = settings.changeIsCropX;
    auto& isCropY = settings.changeIsCropY;
    auto& isSizeX = settings.changeIsSizeX;
    auto& isSizeY = settings.changeIsSizeY;
    auto& isPositionX = settings.changeIsPositionX;
    auto& isPositionY = settings.changeIsPositionY;
    auto& isPivotX = settings.changeIsPivotX;
    auto& isPivotY = settings.changeIsPivotY;
    auto& isScaleX = settings.changeIsScaleX;
    auto& isScaleY = settings.changeIsScaleY;
    auto& isRotation = settings.changeIsRotation;
    auto& isDuration = settings.changeIsDuration;
    auto& isTintR = settings.changeIsTintR;
    auto& isTintG = settings.changeIsTintG;
    auto& isTintB = settings.changeIsTintB;
    auto& isTintA = settings.changeIsTintA;
    auto& isColorOffsetR = settings.changeIsColorOffsetR;
    auto& isColorOffsetG = settings.changeIsColorOffsetG;
    auto& isColorOffsetB = settings.changeIsColorOffsetB;
    auto& isVisibleSet = settings.changeIsVisibleSet;
    auto& isInterpolatedSet = settings.changeIsInterpolatedSet;
    auto& isFlipXSet = settings.changeIsFlipXSet;
    auto& isFlipYSet = settings.changeIsFlipYSet;
    auto& crop = settings.changeCrop;
    auto& size = settings.changeSize;
    auto& position = settings.changePosition;
    auto& pivot = settings.changePivot;
    auto& scale = settings.changeScale;
    auto& rotation = settings.changeRotation;
    auto& duration = settings.changeDuration;
    auto& tint = settings.changeTint;
    auto& colorOffset = settings.changeColorOffset;
    auto& isVisible = settings.changeIsVisible;
    auto& isInterpolated = settings.changeIsInterpolated;
    auto& isFlipX = settings.changeIsFlipX;
    auto& isFlipY = settings.changeIsFlipY;

#define PROPERTIES_WIDGET(body, checkboxLabel, isEnabled)                                                              \
  ImGui::Checkbox(checkboxLabel, &isEnabled);                                                                          \
  ImGui::SameLine();                                                                                                   \
  ImGui::BeginDisabled(!isEnabled);                                                                                    \
  body;                                                                                                                \
  ImGui::EndDisabled();

    auto bool_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, bool& value)
    { PROPERTIES_WIDGET(ImGui::Checkbox(valueLabel, &value), checkboxLabel, isEnabled) };

    auto color3_value = [&](const char* checkboxRLabel, const char* checkboxGLabel, const char* checkboxBLabel,
                            const char* valueRLabel, const char* valueGLabel, const char* valueBLabel,
                            const char* label, bool& isREnabled, bool& isGEnabled, bool& isBEnabled, vec3& value)
    {
      auto style = ImGui::GetStyle();

      auto width = (ImGui::CalcItemWidth() - (ImGui::GetFrameHeightWithSpacing() * 2) - (style.ItemSpacing.x * 2) -
                    ImGui::GetFrameHeight()) /
                   3;

      ivec3 valueAlt = {float_to_uint8(value.r), float_to_uint8(value.g), float_to_uint8(value.b)};

      ImGui::PushItemWidth(width);

      PROPERTIES_WIDGET(ImGui::DragInt(valueRLabel, &valueAlt.r, DRAG_SPEED, 0, 255, "R:%d"), checkboxRLabel,
                        isREnabled);

      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueGLabel, &valueAlt.g, DRAG_SPEED, 0, 255, "G:%d"), checkboxGLabel,
                        isGEnabled);

      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueBLabel, &valueAlt.b, DRAG_SPEED, 0, 255, "B:%d"), checkboxBLabel,
                        isBEnabled);

      ImGui::PopItemWidth();

      ImGui::SameLine();

      value = vec3(uint8_to_float(valueAlt.r), uint8_to_float(valueAlt.g), uint8_to_float(valueAlt.b));

      ImVec4 buttonColor = {isREnabled ? value.r : 0, isGEnabled ? value.g : 0, isBEnabled ? value.b : 0, 1};

      ImGui::ColorButton(label, buttonColor);

      ImGui::SameLine();

      ImGui::TextUnformatted(label);
    };

    auto color4_value = [&](const char* checkboxRLabel, const char* checkboxGLabel, const char* checkboxBLabel,
                            const char* checkboxALabel, const char* valueRLabel, const char* valueGLabel,
                            const char* valueBLabel, const char* valueALabel, const char* label, bool& isREnabled,
                            bool& isGEnabled, bool& isBEnabled, bool& isAEnabled, vec4& value)
    {
      auto style = ImGui::GetStyle();
      auto width = (ImGui::CalcItemWidth() - (ImGui::GetFrameHeightWithSpacing() * 3) - (style.ItemSpacing.x * 3) -
                    ImGui::GetFrameHeight()) /
                   4;
      ivec4 valueAlt = {float_to_uint8(value.r), float_to_uint8(value.g), float_to_uint8(value.b),
                        float_to_uint8(value.a)};

      ImGui::PushItemWidth(width);

      PROPERTIES_WIDGET(ImGui::DragInt(valueRLabel, &valueAlt.r, DRAG_SPEED, 0, 255, "R:%d"), checkboxRLabel,
                        isREnabled);
      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueGLabel, &valueAlt.g, DRAG_SPEED, 0, 255, "G:%d"), checkboxGLabel,
                        isGEnabled);

      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueBLabel, &valueAlt.b, DRAG_SPEED, 0, 255, "B:%d"), checkboxBLabel,
                        isBEnabled);

      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueALabel, &valueAlt.a, DRAG_SPEED, 0, 255, "A:%d"), checkboxALabel,
                        isAEnabled);

      ImGui::PopItemWidth();

      ImGui::SameLine();

      value = vec4(uint8_to_float(valueAlt.r), uint8_to_float(valueAlt.g), uint8_to_float(valueAlt.b),
                   uint8_to_float(valueAlt.a));

      ImVec4 buttonColor = {isREnabled ? value.r : 0, isGEnabled ? value.g : 0, isBEnabled ? value.b : 0,
                            isAEnabled ? value.a : 1};
      ImGui::ColorButton(label, buttonColor);

      ImGui::SameLine();

      ImGui::TextUnformatted(label);
    };

    auto float_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, float& value)
    {
      PROPERTIES_WIDGET(ImGui::DragFloat(valueLabel, &value, DRAG_SPEED, 0.0f, 0.0f, float_format_get(value)),
                        checkboxLabel, isEnabled);
    };

    auto float2_value = [&](const char* checkboxXLabel, const char* checkboxYLabel, const char* valueXLabel,
                            const char* valueYLabel, bool& isXEnabled, bool& isYEnabled, vec2& value)
    {
      auto style = ImGui::GetStyle();

      auto width = (ImGui::CalcItemWidth() - ImGui::GetFrameHeightWithSpacing() - style.ItemSpacing.x) / 2;

      ImGui::PushItemWidth(width);
      PROPERTIES_WIDGET(ImGui::DragFloat(valueXLabel, &value.x, DRAG_SPEED, 0.0f, 0.0f, float_format_get(value.x)),
                        checkboxXLabel, isXEnabled);
      ImGui::SameLine();
      PROPERTIES_WIDGET(ImGui::DragFloat(valueYLabel, &value.y, DRAG_SPEED, 0.0f, 0.0f, float_format_get(value.y)),
                        checkboxYLabel, isYEnabled);
      ImGui::PopItemWidth();
    };

    auto duration_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, int& value)
    {
      PROPERTIES_WIDGET(
          input_int_range(valueLabel, value, anm2::FRAME_DURATION_MIN, anm2::FRAME_DURATION_MAX, STEP, STEP_FAST),
          checkboxLabel, isEnabled);
    };

#undef PROPERTIES_WIDGET

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImGui::GetStyle().ItemInnerSpacing);

    float2_value("##Is Crop X", "##Is Crop Y", "##Crop X", localize.get(BASIC_CROP), isCropX, isCropY, crop);
    float2_value("##Is Size X", "##Is Size Y", "##Size X", localize.get(BASIC_SIZE), isSizeX, isSizeY, size);
    float2_value("##Is Position X", "##Is Position Y", "##Position X", localize.get(BASIC_POSITION), isPositionX,
                 isPositionY, position);
    float2_value("##Is Pivot X", "##Is Pivot Y", "##Pivot X", localize.get(BASIC_PIVOT), isPivotX, isPivotY, pivot);
    float2_value("##Is Scale X", "##Is Scale Y", "##Scale X", localize.get(BASIC_SCALE), isScaleX, isScaleY, scale);
    float_value("##Is Rotation", localize.get(BASIC_ROTATION), isRotation, rotation);
    duration_value("##Is Duration", localize.get(BASIC_DURATION), isDuration, duration);
    color4_value("##Is Tint R", "##Is Tint G", "##Is Tint B", "##Is Tint A", "##Tint R", "##Tint G", "##Tint B",
                 "##Tint A", localize.get(BASIC_TINT), isTintR, isTintG, isTintB, isTintA, tint);
    color3_value("##Is Color Offset R", "##Is Color Offset G", "##Is Color Offset B", "##Color Offset R",
                 "##Color Offset B", "##Color Offset G", localize.get(BASIC_COLOR_OFFSET), isColorOffsetR,
                 isColorOffsetG, isColorOffsetB, colorOffset);
    bool_value("##Is Visible", localize.get(BASIC_VISIBLE), isVisibleSet, isVisible);
    ImGui::SameLine();
    bool_value("##Is Interpolated", localize.get(BASIC_INTERPOLATED), isInterpolatedSet, isInterpolated);

    bool_value("##Is Flip X", localize.get(LABEL_FLIP_X), isFlipXSet, isFlipX);
    ImGui::SameLine();
    bool_value("##Is Flip Y", localize.get(LABEL_FLIP_Y), isFlipYSet, isFlipY);

    ImGui::PopStyleVar();

    auto frame_change = [&](anm2::ChangeType type)
    {
      anm2::FrameChange frameChange;
      if (isCropX) frameChange.cropX = crop.x;
      if (isCropY) frameChange.cropY = crop.y;
      if (isSizeX) frameChange.sizeX = size.x;
      if (isSizeY) frameChange.sizeY = size.y;
      if (isPositionX) frameChange.positionX = position.x;
      if (isPositionY) frameChange.positionY = position.y;
      if (isPivotX) frameChange.pivotX = pivot.x;
      if (isPivotY) frameChange.pivotY = pivot.y;
      if (isScaleX) frameChange.scaleX = scale.x;
      if (isScaleY) frameChange.scaleY = scale.y;
      if (isRotation) frameChange.rotation = std::make_optional(rotation);
      if (isDuration) frameChange.duration = std::make_optional(duration);
      if (isTintR) frameChange.tintR = tint.r;
      if (isTintG) frameChange.tintG = tint.g;
      if (isTintB) frameChange.tintB = tint.b;
      if (isTintA) frameChange.tintA = tint.a;
      if (isColorOffsetR) frameChange.colorOffsetR = colorOffset.r;
      if (isColorOffsetG) frameChange.colorOffsetG = colorOffset.g;
      if (isColorOffsetB) frameChange.colorOffsetB = colorOffset.b;
      if (isVisibleSet) frameChange.isVisible = std::make_optional(isVisible);
      if (isInterpolatedSet) frameChange.isInterpolated = std::make_optional(isInterpolated);
      if (isFlipXSet) frameChange.isFlipX = std::make_optional(isFlipX);
      if (isFlipYSet) frameChange.isFlipY = std::make_optional(isFlipY);

      DOCUMENT_EDIT(document, localize.get(EDIT_CHANGE_FRAME_PROPERTIES), Document::FRAMES,
                    document.item_get()->frames_change(frameChange, type, frames));

      isChanged = true;
    };

    ImGui::Separator();

    bool isAnyProperty = isCropX || isCropY || isSizeX || isSizeY || isPositionX || isPositionY || isPivotX ||
                         isPivotY || isScaleX || isScaleY || isRotation || isDuration || isTintR || isTintG ||
                         isTintB || isTintA || isColorOffsetR || isColorOffsetG || isColorOffsetB || isVisibleSet ||
                         isInterpolatedSet || isFlipXSet || isFlipYSet;

    auto rowWidgetSize = widget_size_with_row_get(5);

    ImGui::BeginDisabled(!isAnyProperty);

    if (ImGui::Button(localize.get(LABEL_ADJUST), rowWidgetSize)) frame_change(anm2::ADJUST);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADJUST));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(BASIC_ADD), rowWidgetSize)) frame_change(anm2::ADD);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADD_VALUES));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_SUBTRACT), rowWidgetSize)) frame_change(anm2::SUBTRACT);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SUBTRACT_VALUES));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_MULTIPLY), rowWidgetSize)) frame_change(anm2::MULTIPLY);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MULTIPLY_VALUES));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_DIVIDE), rowWidgetSize)) frame_change(anm2::DIVIDE);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_DIVIDE_VALUES));

    ImGui::EndDisabled();
  }
}
