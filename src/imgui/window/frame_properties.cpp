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
      auto& type = document.reference.itemType;
      auto frame = document.frame_get();
      auto useFrame = frame ? *frame : anm2::Frame();

      ImGui::BeginDisabled(!frame);
      {
        if (type == anm2::TRIGGER)
        {
          if (combo_negative_one_indexed("Event", frame ? &useFrame.eventID : &dummy_value<int>(),
                                         document.event.labels))
            DOCUMENT_EDIT(document, "Trigger Event", Document::FRAMES, frame->eventID = useFrame.eventID);
          ImGui::SetItemTooltip("Change the event this trigger uses.");

          if (combo_negative_one_indexed("Sound", frame ? &useFrame.soundID : &dummy_value<int>(),
                                         document.sound.labels))
            DOCUMENT_EDIT(document, "Trigger Sound", Document::FRAMES, frame->soundID = useFrame.soundID);
          ImGui::SetItemTooltip("Change the sound this trigger uses.");

          if (ImGui::InputInt("At Frame", frame ? &useFrame.atFrame : &dummy_value<int>(), imgui::STEP,
                              imgui::STEP_FAST, !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0))
            DOCUMENT_EDIT(document, "Trigger At Frame", Document::FRAMES, frame->atFrame = useFrame.atFrame);
          ImGui::SetItemTooltip("Change the frame the trigger will be activated at.");
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

          if (ImGui::InputFloat("Rotation", frame ? &useFrame.rotation : &dummy_value<float>(), imgui::STEP,
                                imgui::STEP_FAST, frame ? float_format_get(useFrame.rotation) : ""))
            DOCUMENT_EDIT(document, "Frame Rotation", Document::FRAMES, frame->rotation = useFrame.rotation);
          ImGui::SetItemTooltip("Change the rotation of the frame.");

          if (ImGui::InputInt("Duration", frame ? &useFrame.delay : &dummy_value<int>(), imgui::STEP, imgui::STEP_FAST,
                              !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0))
            DOCUMENT_EDIT(document, "Frame Duration", Document::FRAMES, frame->delay = useFrame.delay);
          ImGui::SetItemTooltip("Change how long the frame lasts.");

          if (ImGui::ColorEdit4("Tint", frame ? value_ptr(useFrame.tint) : &dummy_value<float>()))
            DOCUMENT_EDIT(document, "Frame Tint", Document::FRAMES, frame->tint = useFrame.tint);
          ImGui::SetItemTooltip("Change the tint of the frame.");

          if (ImGui::ColorEdit3("Color Offset", frame ? value_ptr(useFrame.colorOffset) : &dummy_value<float>()))
            DOCUMENT_EDIT(document, "Frame Color Offset", Document::FRAMES, frame->colorOffset = useFrame.colorOffset);
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

          auto widgetSize = imgui::widget_size_with_row_get(2);

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
    ImGui::End();
  }
}