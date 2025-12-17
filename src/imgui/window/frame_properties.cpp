#include "frame_properties.h"

#include <glm/gtc/type_ptr.hpp>
#include <limits>

#include "math_.h"
#include "strings.h"
#include "types.h"

using namespace anm2ed::util::math;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::imgui
{

  void FrameProperties::update(Manager& manager, Settings& settings)
  {
    if (ImGui::Begin(localize.get(LABEL_FRAME_PROPERTIES_WINDOW), &settings.windowIsFrameProperties))
    {
      auto& document = *manager.get();
      auto& frames = document.frames.selection;
      auto& type = document.reference.itemType;

      if (frames.size() <= 1)
      {
        auto frame = document.frame_get();
        auto useFrame = frame ? *frame : anm2::Frame();

        ImGui::BeginDisabled(!frame);
        {
          if (type == anm2::TRIGGER)
          {
            if (combo_negative_one_indexed(localize.get(BASIC_EVENT),
                                           frame ? &useFrame.eventID : &dummy_value_negative<int>(),
                                           document.event.labels) &&
                frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_EVENT), Document::FRAMES,
                            frame->eventID = useFrame.eventID);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_EVENT));

            if (combo_negative_one_indexed(localize.get(BASIC_SOUND),
                                           frame ? &useFrame.soundID : &dummy_value_negative<int>(),
                                           document.sound.labels) &&
                frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_SOUND), Document::FRAMES,
                            frame->soundID = useFrame.soundID);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_SOUND));

            if (input_int_range(localize.get(BASIC_AT_FRAME), frame ? useFrame.atFrame : dummy_value<int>(), 0,
                                std::numeric_limits<int>::max(), STEP, STEP_FAST,
                                !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0) &&
                frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_AT_FRAME), Document::FRAMES,
                            frame->atFrame = useFrame.atFrame);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_AT_FRAME));

            if (ImGui::Checkbox(localize.get(BASIC_VISIBLE), frame ? &useFrame.isVisible : &dummy_value<bool>()) &&
                frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_VISIBILITY), Document::FRAMES,
                            frame->isVisible = useFrame.isVisible);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_VISIBILITY));
          }
          else
          {
            ImGui::BeginDisabled(type == anm2::ROOT || type == anm2::NULL_);
            {
              auto cropEdit =
                  drag_float2_persistent(localize.get(BASIC_CROP), frame ? &frame->crop : &dummy_value<vec2>(),
                                         DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(frame->crop) : "");
              if (cropEdit == edit::START)
                document.snapshot(localize.get(EDIT_FRAME_CROP));
              else if (cropEdit == edit::END)
                document.change(Document::FRAMES);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CROP));

              auto sizeEdit =
                  drag_float2_persistent(localize.get(BASIC_SIZE), frame ? &frame->size : &dummy_value<vec2>(),
                                         DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(frame->size) : "");
              if (sizeEdit == edit::START)
                document.snapshot(localize.get(EDIT_FRAME_SIZE));
              else if (sizeEdit == edit::END)
                document.change(Document::FRAMES);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SIZE));
            }
            ImGui::EndDisabled();

            auto positionEdit =
                drag_float2_persistent(localize.get(BASIC_POSITION), frame ? &frame->position : &dummy_value<vec2>(),
                                       DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(frame->position) : "");
            if (positionEdit == edit::START)
              document.snapshot(localize.get(EDIT_FRAME_POSITION));
            else if (positionEdit == edit::END)
              document.change(Document::FRAMES);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_POSITION));

            ImGui::BeginDisabled(type == anm2::ROOT || type == anm2::NULL_);
            {
              auto pivotEdit =
                  drag_float2_persistent(localize.get(BASIC_PIVOT), frame ? &frame->pivot : &dummy_value<vec2>(),
                                         DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(frame->pivot) : "");
              if (pivotEdit == edit::START)
                document.snapshot(localize.get(EDIT_FRAME_PIVOT));
              else if (pivotEdit == edit::END)
                document.change(Document::FRAMES);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PIVOT));
            }
            ImGui::EndDisabled();

            auto scaleEdit =
                drag_float2_persistent(localize.get(BASIC_SCALE), frame ? &frame->scale : &dummy_value<vec2>(),
                                       DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(frame->scale) : "");
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SCALE));
            if (scaleEdit == edit::START)
              document.snapshot(localize.get(EDIT_FRAME_SCALE));
            else if (scaleEdit == edit::END)
              document.change(Document::FRAMES);

            auto rotationEdit =
                drag_float_persistent(localize.get(BASIC_ROTATION), frame ? &frame->rotation : &dummy_value<float>(),
                                      DRAG_SPEED, 0.0f, 0.0f, frame ? float_format_get(frame->rotation) : "");
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROTATION));
            if (rotationEdit == edit::START)
              document.snapshot(localize.get(EDIT_FRAME_ROTATION));
            else if (rotationEdit == edit::END)
              document.change(Document::FRAMES);

            if (input_int_range(localize.get(BASIC_DURATION), frame ? useFrame.duration : dummy_value<int>(),
                                frame ? anm2::FRAME_DURATION_MIN : 0, anm2::FRAME_DURATION_MAX, STEP, STEP_FAST,
                                !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0) &&
                frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_DURATION), Document::FRAMES,
                            frame->duration = useFrame.duration);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_DURATION));

            auto tintEdit =
                color_edit4_persistent(localize.get(BASIC_TINT), frame ? &frame->tint : &dummy_value<vec4>());
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TINT));
            if (tintEdit == edit::START)
              document.snapshot(localize.get(EDIT_FRAME_TINT));
            else if (tintEdit == edit::END)
              document.change(Document::FRAMES);

            auto colorOffsetEdit = color_edit3_persistent(localize.get(BASIC_COLOR_OFFSET),
                                                          frame ? &frame->colorOffset : &dummy_value<vec3>());
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_COLOR_OFFSET));
            if (colorOffsetEdit == edit::START)
              document.snapshot(localize.get(EDIT_FRAME_COLOR_OFFSET));
            else if (colorOffsetEdit == edit::END)
              document.change(Document::FRAMES);

            if (ImGui::Checkbox(localize.get(BASIC_VISIBLE), frame ? &useFrame.isVisible : &dummy_value<bool>()) &&
                frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_VISIBILITY), Document::FRAMES,
                            frame->isVisible = useFrame.isVisible);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FRAME_VISIBILITY));

            ImGui::SameLine();

            if (ImGui::Checkbox(localize.get(BASIC_INTERPOLATED),
                                frame ? &useFrame.isInterpolated : &dummy_value<bool>()) &&
                frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_INTERPOLATION), Document::FRAMES,
                            frame->isInterpolated = useFrame.isInterpolated);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FRAME_INTERPOLATION));

            auto widgetSize = widget_size_with_row_get(2);

            if (ImGui::Button(localize.get(LABEL_FLIP_X), widgetSize) && frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_FLIP_X), Document::FRAMES,
                            frame->scale.x = -frame->scale.x);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FLIP_X));
            ImGui::SameLine();
            if (ImGui::Button(localize.get(LABEL_FLIP_Y), widgetSize) && frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_FLIP_Y), Document::FRAMES,
                            frame->scale.y = -frame->scale.y);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FLIP_Y));
          }
        }
        ImGui::EndDisabled();
      }
      else
      {
        auto& isCrop = settings.changeIsCrop;
        auto& isCropX = settings.changeIsCropX;
        auto& isCropY = settings.changeIsCropY;
        auto& isSize = settings.changeIsSize;
        auto& isPosition = settings.changeIsPosition;
        auto& isPivot = settings.changeIsPivot;
        auto& isScale = settings.changeIsScale;
        auto& isRotation = settings.changeIsRotation;
        auto& isDuration = settings.changeIsDuration;
        auto& isTint = settings.changeIsTint;
        auto& isColorOffset = settings.changeIsColorOffset;
        auto& isVisibleSet = settings.changeIsVisibleSet;
        auto& isInterpolatedSet = settings.changeIsInterpolatedSet;
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

#define PROPERTIES_WIDGET(body, checkboxLabel, isEnabled)                                                              \
  ImGui::Checkbox(checkboxLabel, &isEnabled);                                                                          \
  ImGui::SameLine();                                                                                                   \
  ImGui::BeginDisabled(!isEnabled);                                                                                    \
  body;                                                                                                                \
  ImGui::EndDisabled();

        auto bool_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, bool& value)
        { PROPERTIES_WIDGET(ImGui::Checkbox(valueLabel, &value), checkboxLabel, isEnabled) };

        auto color3_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec3& value)
        { PROPERTIES_WIDGET(ImGui::ColorEdit3(valueLabel, value_ptr(value)), checkboxLabel, isEnabled); };

        auto color4_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec4& value)
        { PROPERTIES_WIDGET(ImGui::ColorEdit4(valueLabel, value_ptr(value)), checkboxLabel, isEnabled); };

        auto float2_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec2& value)
        {
          PROPERTIES_WIDGET(ImGui::InputFloat2(valueLabel, value_ptr(value), vec2_format_get(value)), checkboxLabel,
                            isEnabled);
        };

        auto float2_value_new = [&](const char* checkboxXLabel, const char* checkboxYLabel, const char* valueXLabel,
                                    const char* valueYLabel, bool& isXEnabled, bool& isYEnabled, vec2& value)
        {
          auto width =
              (ImGui::CalcItemWidth() - ImGui::GetTextLineHeight() - (ImGui::GetStyle().ItemInnerSpacing.x * 6)) / 2;

          PROPERTIES_WIDGET(ImGui::PushItemWidth(width);
                            ImGui::DragFloat(valueXLabel, &value.x, DRAG_SPEED, 0.0f, 0.0f, float_format_get(value.x));
                            ImGui::PopItemWidth(), checkboxXLabel, isXEnabled);
          ImGui::SameLine();
          PROPERTIES_WIDGET(ImGui::PushItemWidth(width);
                            ImGui::DragFloat(valueYLabel, &value.y, DRAG_SPEED, 0.0f, 0.0f, float_format_get(value.y));
                            ImGui::PopItemWidth(), checkboxYLabel, isYEnabled);
        };

        auto float_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, float& value)
        {
          PROPERTIES_WIDGET(ImGui::InputFloat(valueLabel, &value, STEP, STEP_FAST, float_format_get(value)),
                            checkboxLabel, isEnabled);
        };

        auto duration_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, int& value)
        {
          PROPERTIES_WIDGET(
              input_int_range(valueLabel, value, anm2::FRAME_DURATION_MIN, anm2::FRAME_DURATION_MAX, STEP, STEP_FAST),
              checkboxLabel, isEnabled);
        };

#undef PROPERTIES_WIDGET

        float2_value_new("##Is Crop X", "##Is Crop Y", "##Crop X", localize.get(BASIC_CROP), isCropX, isCropY, crop);
        float2_value("##Is Size", localize.get(BASIC_SIZE), isSize, size);
        float2_value("##Is Position", localize.get(BASIC_POSITION), isPosition, position);
        float2_value("##Is Pivot", localize.get(BASIC_PIVOT), isPivot, pivot);
        float2_value("##Is Scale", localize.get(BASIC_SCALE), isScale, scale);
        float_value("##Is Rotation", localize.get(BASIC_ROTATION), isRotation, rotation);
        duration_value("##Is Duration", localize.get(BASIC_DURATION), isDuration, duration);
        color4_value("##Is Tint", localize.get(BASIC_TINT), isTint, tint);
        color3_value("##Is Color Offset", localize.get(BASIC_COLOR_OFFSET), isColorOffset, colorOffset);
        bool_value("##Is Visible", localize.get(BASIC_VISIBLE), isVisibleSet, isVisible);
        ImGui::SameLine();
        bool_value("##Is Interpolated", localize.get(BASIC_INTERPOLATED), isInterpolatedSet, isInterpolated);

        auto frame_change = [&](anm2::ChangeType type)
        {
          anm2::FrameChange frameChange;
          if (isCrop) frameChange.crop = std::make_optional(crop);
          if (isSize) frameChange.size = std::make_optional(size);
          if (isPosition) frameChange.position = std::make_optional(position);
          if (isPivot) frameChange.pivot = std::make_optional(pivot);
          if (isScale) frameChange.scale = std::make_optional(scale);
          if (isRotation) frameChange.rotation = std::make_optional(rotation);
          if (isDuration) frameChange.duration = std::make_optional(duration);
          if (isTint) frameChange.tint = std::make_optional(tint);
          if (isColorOffset) frameChange.colorOffset = std::make_optional(colorOffset);
          if (isVisibleSet) frameChange.isVisible = std::make_optional(isVisible);
          if (isInterpolatedSet) frameChange.isInterpolated = std::make_optional(isInterpolated);

          DOCUMENT_EDIT(document, localize.get(EDIT_CHANGE_FRAME_PROPERTIES), Document::FRAMES,
                        document.item_get()->frames_change(frameChange, type, *frames.begin(), (int)frames.size()));
        };

        ImGui::Separator();

        bool isAnyProperty = isCrop || isSize || isPosition || isPivot || isScale || isRotation || isDuration ||
                             isTint || isColorOffset || isVisibleSet || isInterpolatedSet;

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
    ImGui::End();
  }
}
