#include "frame_properties.h"

#include <glm/gtc/type_ptr.hpp>

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
                                           document.event.labels))
              DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_EVENT), Document::FRAMES,
                            frame->eventID = useFrame.eventID);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_EVENT));

            if (combo_negative_one_indexed(localize.get(BASIC_SOUND),
                                           frame ? &useFrame.soundID : &dummy_value_negative<int>(),
                                           document.sound.labels))
              DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_SOUND), Document::FRAMES,
                            frame->soundID = useFrame.soundID);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_SOUND));

            if (ImGui::InputInt(localize.get(BASIC_AT_FRAME), frame ? &useFrame.atFrame : &dummy_value<int>(), STEP,
                                STEP_FAST, !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0))
              DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_AT_FRAME), Document::FRAMES,
                            frame->atFrame = useFrame.atFrame);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_AT_FRAME));

            if (ImGui::Checkbox(localize.get(BASIC_VISIBLE), frame ? &useFrame.isVisible : &dummy_value<bool>()))
              DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_VISIBILITY), Document::FRAMES,
                            frame->isVisible = useFrame.isVisible);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_VISIBILITY));
          }
          else
          {
            ImGui::BeginDisabled(type == anm2::ROOT || type == anm2::NULL_);
            {
              if (ImGui::InputFloat2(localize.get(BASIC_CROP), frame ? value_ptr(useFrame.crop) : &dummy_value<float>(),
                                     frame ? vec2_format_get(useFrame.crop) : ""))
                DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_CROP), Document::FRAMES, frame->crop = useFrame.crop);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CROP));

              if (ImGui::InputFloat2(localize.get(BASIC_SIZE), frame ? value_ptr(useFrame.size) : &dummy_value<float>(),
                                     frame ? vec2_format_get(useFrame.size) : ""))
                DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_SIZE), Document::FRAMES, frame->size = useFrame.size);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SIZE));
            }
            ImGui::EndDisabled();

            if (ImGui::InputFloat2(localize.get(BASIC_POSITION),
                                   frame ? value_ptr(useFrame.position) : &dummy_value<float>(),
                                   frame ? vec2_format_get(useFrame.position) : ""))
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_POSITION), Document::FRAMES,
                            frame->position = useFrame.position);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_POSITION));

            ImGui::BeginDisabled(type == anm2::ROOT || type == anm2::NULL_);
            {
              if (ImGui::InputFloat2(localize.get(BASIC_PIVOT),
                                     frame ? value_ptr(useFrame.pivot) : &dummy_value<float>(),
                                     frame ? vec2_format_get(useFrame.pivot) : ""))
                DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_PIVOT), Document::FRAMES,
                              frame->pivot = useFrame.pivot);
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PIVOT));
            }
            ImGui::EndDisabled();

            if (ImGui::InputFloat2(localize.get(BASIC_SCALE), frame ? value_ptr(useFrame.scale) : &dummy_value<float>(),
                                   frame ? vec2_format_get(useFrame.scale) : ""))
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_SCALE), Document::FRAMES, frame->scale = useFrame.scale);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SCALE));

            if (ImGui::InputFloat(localize.get(BASIC_ROTATION), frame ? &useFrame.rotation : &dummy_value<float>(),
                                  STEP, STEP_FAST, frame ? float_format_get(useFrame.rotation) : ""))
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_ROTATION), Document::FRAMES,
                            frame->rotation = useFrame.rotation);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROTATION));

            if (input_int_range(localize.get(BASIC_DURATION), frame ? useFrame.duration : dummy_value<int>(),
                                frame ? anm2::FRAME_DURATION_MIN : 0, anm2::FRAME_DURATION_MAX, STEP, STEP_FAST,
                                !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0))
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_DURATION), Document::FRAMES,
                            frame->duration = useFrame.duration);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_DURATION));

            if (ImGui::ColorEdit4(localize.get(BASIC_TINT), frame ? value_ptr(useFrame.tint) : &dummy_value<float>()))
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_TINT), Document::FRAMES, frame->tint = useFrame.tint);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TINT));

            if (ImGui::ColorEdit3(localize.get(BASIC_COLOR_OFFSET),
                                  frame ? value_ptr(useFrame.colorOffset) : &dummy_value<float>()))
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_COLOR_OFFSET), Document::FRAMES,
                            frame->colorOffset = useFrame.colorOffset);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_COLOR_OFFSET));

            if (ImGui::Checkbox(localize.get(BASIC_VISIBLE), frame ? &useFrame.isVisible : &dummy_value<bool>()))
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_VISIBILITY), Document::FRAMES,
                            frame->isVisible = useFrame.isVisible);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FRAME_VISIBILITY));

            ImGui::SameLine();

            if (ImGui::Checkbox(localize.get(BASIC_INTERPOLATED),
                                frame ? &useFrame.isInterpolated : &dummy_value<bool>()))
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_INTERPOLATION), Document::FRAMES,
                            frame->isInterpolated = useFrame.isInterpolated);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FRAME_INTERPOLATION));

            auto widgetSize = widget_size_with_row_get(2);

            if (ImGui::Button(localize.get(LABEL_FLIP_X), widgetSize))
              DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_FLIP_X), Document::FRAMES,
                            frame->scale.x = -frame->scale.x);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FLIP_X));
            ImGui::SameLine();
            if (ImGui::Button(localize.get(LABEL_FLIP_Y), widgetSize))
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

#define PROPERTIES_WIDGET(body)                                                                                        \
  ImGui::Checkbox(checkboxLabel, &isEnabled);                                                                          \
  ImGui::SameLine();                                                                                                   \
  ImGui::BeginDisabled(!isEnabled);                                                                                    \
  body;                                                                                                                \
  ImGui::EndDisabled();

        auto bool_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, bool& value)
        { PROPERTIES_WIDGET(ImGui::Checkbox(valueLabel, &value)); };

        auto color3_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec3& value)
        { PROPERTIES_WIDGET(ImGui::ColorEdit3(valueLabel, value_ptr(value))); };

        auto color4_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec4& value)
        { PROPERTIES_WIDGET(ImGui::ColorEdit4(valueLabel, value_ptr(value))); };

        auto float2_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec2& value)
        { PROPERTIES_WIDGET(ImGui::InputFloat2(valueLabel, value_ptr(value), vec2_format_get(value))); };

        auto float_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, float& value)
        { PROPERTIES_WIDGET(ImGui::InputFloat(valueLabel, &value, STEP, STEP_FAST, float_format_get(value))); };

        auto duration_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, int& value)
        {
          PROPERTIES_WIDGET(
              input_int_range(valueLabel, value, anm2::FRAME_DURATION_MIN, anm2::FRAME_DURATION_MAX, STEP, STEP_FAST));
        };

#undef PROPERTIES_WIDGET

        float2_value("##Is Crop", localize.get(BASIC_CROP), isCrop, crop);
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

        auto rowOneWidgetSize = widget_size_with_row_get(1);

        if (ImGui::Button(localize.get(LABEL_ADJUST), rowOneWidgetSize)) frame_change(anm2::ADJUST);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADJUST));

        auto rowTwoWidgetSize = widget_size_with_row_get(4);

        if (ImGui::Button(localize.get(BASIC_ADD), rowTwoWidgetSize)) frame_change(anm2::ADD);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADD_VALUES));
        ImGui::SameLine();
        if (ImGui::Button(localize.get(LABEL_SUBTRACT), rowTwoWidgetSize)) frame_change(anm2::SUBTRACT);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SUBTRACT_VALUES));
        ImGui::SameLine();
        if (ImGui::Button(localize.get(LABEL_MULTIPLY), rowTwoWidgetSize)) frame_change(anm2::MULTIPLY);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MULTIPLY_VALUES));
        ImGui::SameLine();
        if (ImGui::Button(localize.get(LABEL_DIVIDE), rowTwoWidgetSize)) frame_change(anm2::DIVIDE);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_DIVIDE_VALUES));
      }
    }
    ImGui::End();
  }
}
