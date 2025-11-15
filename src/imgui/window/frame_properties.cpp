#include "frame_properties.h"

#include <glm/gtc/type_ptr.hpp>

#include "math_.h"
#include "types.h"

using namespace anm2ed::util::math;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::imgui
{
  void FrameProperties::update(Manager& manager, Settings& settings)
  {
    if (ImGui::Begin("Frame Properties", &settings.windowIsFrameProperties))
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
            if (combo_negative_one_indexed("Event", frame ? &useFrame.eventID : &dummy_value_negative<int>(),
                                           document.event.labels))
              DOCUMENT_EDIT(document, "Trigger Event", Document::FRAMES, frame->eventID = useFrame.eventID);
            ImGui::SetItemTooltip("Change the event this trigger uses.");

            if (combo_negative_one_indexed("Sound", frame ? &useFrame.soundID : &dummy_value_negative<int>(),
                                           document.sound.labels))
              DOCUMENT_EDIT(document, "Trigger Sound", Document::FRAMES, frame->soundID = useFrame.soundID);
            ImGui::SetItemTooltip("Change the sound this trigger uses.");

            if (ImGui::InputInt("At Frame", frame ? &useFrame.atFrame : &dummy_value<int>(), STEP, STEP_FAST,
                                !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0))
              DOCUMENT_EDIT(document, "Trigger At Frame", Document::FRAMES, frame->atFrame = useFrame.atFrame);
            ImGui::SetItemTooltip("Change the frame the trigger will be activated at.");

            if (ImGui::Checkbox("Visible", frame ? &useFrame.isVisible : &dummy_value<bool>()))
              DOCUMENT_EDIT(document, "Trigger Visibility", Document::FRAMES, frame->isVisible = useFrame.isVisible);
            ImGui::SetItemTooltip("Toggle the trigger's visibility.");
          }
          else
          {
            ImGui::BeginDisabled(type == anm2::ROOT || type == anm2::NULL_);
            {
              if (ImGui::InputFloat2("Crop", frame ? value_ptr(useFrame.crop) : &dummy_value<float>(),
                                     frame ? vec2_format_get(useFrame.crop) : ""))
                DOCUMENT_EDIT(document, "Frame Crop", Document::FRAMES, frame->crop = useFrame.crop);
              ImGui::SetItemTooltip("Change the crop position the frame uses.");

              if (ImGui::InputFloat2("Size", frame ? value_ptr(useFrame.size) : &dummy_value<float>(),
                                     frame ? vec2_format_get(useFrame.size) : ""))
                DOCUMENT_EDIT(document, "Frame Size", Document::FRAMES, frame->size = useFrame.size);
              ImGui::SetItemTooltip("Change the size of the crop the frame uses.");
            }
            ImGui::EndDisabled();

            if (ImGui::InputFloat2("Position", frame ? value_ptr(useFrame.position) : &dummy_value<float>(),
                                   frame ? vec2_format_get(useFrame.position) : ""))
              DOCUMENT_EDIT(document, "Frame Position", Document::FRAMES, frame->position = useFrame.position);
            ImGui::SetItemTooltip("Change the position of the frame.");

            ImGui::BeginDisabled(type == anm2::ROOT || type == anm2::NULL_);
            {
              if (ImGui::InputFloat2("Pivot", frame ? value_ptr(useFrame.pivot) : &dummy_value<float>(),
                                     frame ? vec2_format_get(useFrame.pivot) : ""))
                DOCUMENT_EDIT(document, "Frame Pivot", Document::FRAMES, frame->pivot = useFrame.pivot);
              ImGui::SetItemTooltip("Change the pivot of the frame; i.e., where it is centered.");
            }
            ImGui::EndDisabled();

            if (ImGui::InputFloat2("Scale", frame ? value_ptr(useFrame.scale) : &dummy_value<float>(),
                                   frame ? vec2_format_get(useFrame.scale) : ""))
              DOCUMENT_EDIT(document, "Frame Scale", Document::FRAMES, frame->scale = useFrame.scale);
            ImGui::SetItemTooltip("Change the scale of the frame, in percent.");

            if (ImGui::InputFloat("Rotation", frame ? &useFrame.rotation : &dummy_value<float>(), STEP, STEP_FAST,
                                  frame ? float_format_get(useFrame.rotation) : ""))
              DOCUMENT_EDIT(document, "Frame Rotation", Document::FRAMES, frame->rotation = useFrame.rotation);
            ImGui::SetItemTooltip("Change the rotation of the frame.");

            if (input_int_range("Duration", frame ? useFrame.duration : dummy_value<int>(),
                                frame ? anm2::FRAME_DURATION_MIN : 0, anm2::FRAME_DURATION_MAX, STEP, STEP_FAST,
                                !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0))
              DOCUMENT_EDIT(document, "Frame Duration", Document::FRAMES, frame->duration = useFrame.duration);
            ImGui::SetItemTooltip("Change how long the frame lasts.");

            if (ImGui::ColorEdit4("Tint", frame ? value_ptr(useFrame.tint) : &dummy_value<float>()))
              DOCUMENT_EDIT(document, "Frame Tint", Document::FRAMES, frame->tint = useFrame.tint);
            ImGui::SetItemTooltip("Change the tint of the frame.");

            if (ImGui::ColorEdit3("Color Offset", frame ? value_ptr(useFrame.colorOffset) : &dummy_value<float>()))
              DOCUMENT_EDIT(document, "Frame Color Offset", Document::FRAMES,
                            frame->colorOffset = useFrame.colorOffset);
            ImGui::SetItemTooltip("Change the color added onto the frame.");

            if (ImGui::Checkbox("Visible", frame ? &useFrame.isVisible : &dummy_value<bool>()))
              DOCUMENT_EDIT(document, "Frame Visibility", Document::FRAMES, frame->isVisible = useFrame.isVisible);
            ImGui::SetItemTooltip("Toggle the frame's visibility.");

            ImGui::SameLine();

            if (ImGui::Checkbox("Interpolated", frame ? &useFrame.isInterpolated : &dummy_value<bool>()))
              DOCUMENT_EDIT(document, "Frame Interpolation", Document::FRAMES,
                            frame->isInterpolated = useFrame.isInterpolated);
            ImGui::SetItemTooltip(
                "Toggle the frame interpolating; i.e., blending its values into the next frame based on the time.");

            auto widgetSize = widget_size_with_row_get(2);

            if (ImGui::Button("Flip X", widgetSize))
              DOCUMENT_EDIT(document, "Frame Flip X", Document::FRAMES, frame->scale.x = -frame->scale.x);
            ImGui::SetItemTooltip("%s", "Flip the horizontal scale of the frame, to cheat mirroring the frame "
                                        "horizontally.\n(Note: the format does not support mirroring.)");
            ImGui::SameLine();
            if (ImGui::Button("Flip Y", widgetSize))
              DOCUMENT_EDIT(document, "Frame Flip Y", Document::FRAMES, frame->scale.y = -frame->scale.y);
            ImGui::SetItemTooltip("%s", "Flip the vertical scale of the frame, to cheat mirroring the frame "
                                        "vertically.\n(Note: the format does not support mirroring.)");
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
        auto& isDelay = settings.changeIsDelay;
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
        auto& duration = settings.changeDelay;
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

        float2_value("##Is Crop", "Crop", isCrop, crop);
        float2_value("##Is Size", "Size", isSize, size);
        float2_value("##Is Position", "Position", isPosition, position);
        float2_value("##Is Pivot", "Pivot", isPivot, pivot);
        float2_value("##Is Scale", "Scale", isScale, scale);
        float_value("##Is Rotation", "Rotation", isRotation, rotation);
        duration_value("##Is Delay", "Delay", isDelay, duration);
        color4_value("##Is Tint", "Tint", isTint, tint);
        color3_value("##Is Color Offset", "Color Offset", isColorOffset, colorOffset);
        bool_value("##Is Visible", "Visible", isVisibleSet, isVisible);
        ImGui::SameLine();
        bool_value("##Is Interpolated", "Interpolated", isInterpolatedSet, isInterpolated);

        auto frame_change = [&](anm2::ChangeType type)
        {
          anm2::FrameChange frameChange;
          if (isCrop) frameChange.crop = std::make_optional(crop);
          if (isSize) frameChange.size = std::make_optional(size);
          if (isPosition) frameChange.position = std::make_optional(position);
          if (isPivot) frameChange.pivot = std::make_optional(pivot);
          if (isScale) frameChange.scale = std::make_optional(scale);
          if (isRotation) frameChange.rotation = std::make_optional(rotation);
          if (isDelay) frameChange.duration = std::make_optional(duration);
          if (isTint) frameChange.tint = std::make_optional(tint);
          if (isColorOffset) frameChange.colorOffset = std::make_optional(colorOffset);
          if (isVisibleSet) frameChange.isVisible = std::make_optional(isVisible);
          if (isInterpolatedSet) frameChange.isInterpolated = std::make_optional(isInterpolated);

          DOCUMENT_EDIT(document, "Change Frame Properties", Document::FRAMES,
                        document.item_get()->frames_change(frameChange, type, *frames.begin(), (int)frames.size()));
        };

        auto rowOneWidgetSize = widget_size_with_row_get(1);

        if (ImGui::Button("Adjust", rowOneWidgetSize)) frame_change(anm2::ADJUST);
        ImGui::SetItemTooltip("Set the value of each specified value onto the frame's equivalent.");

        auto rowTwoWidgetSize = widget_size_with_row_get(4);

        if (ImGui::Button("Add", rowTwoWidgetSize)) frame_change(anm2::ADD);
        ImGui::SetItemTooltip("Add the specified values onto each frame.\n(Boolean values will simply be set.)");
        ImGui::SameLine();
        if (ImGui::Button("Subtract", rowTwoWidgetSize)) frame_change(anm2::SUBTRACT);
        ImGui::SetItemTooltip("Subtract the specified values from each frame.\n(Boolean values will simply be set.)");
        ImGui::SameLine();
        if (ImGui::Button("Multiply", rowTwoWidgetSize)) frame_change(anm2::MULTIPLY);
        ImGui::SetItemTooltip("Multiply the specified values for each frame.\n(Boolean values will simply be set.)");
        ImGui::SameLine();
        if (ImGui::Button("Divide", rowTwoWidgetSize)) frame_change(anm2::DIVIDE);
        ImGui::SetItemTooltip("Divide the specified values for each frame.\n(Boolean values will simply be set.)");
      }
    }
    ImGui::End();
  }
}