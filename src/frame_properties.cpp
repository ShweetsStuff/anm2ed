#include "frame_properties.h"

#include <ranges>

#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "math.h"
#include "types.h"

using namespace anm2ed::settings;
using namespace anm2ed::manager;
using namespace anm2ed::math;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::frame_properties
{

  void FrameProperties::update(Manager& manager, Settings& settings)
  {
    if (ImGui::Begin("Frame Properties", &settings.windowIsFrameProperties))
    {
      auto& document = *manager.get();
      auto& anm2 = document.anm2;
      auto& reference = document.reference;
      auto& type = reference.itemType;
      auto frame = document.frame_get();
      auto useFrame = frame ? *frame : anm2::Frame();

      ImGui::BeginDisabled(!frame);
      {
        if (type == anm2::TRIGGER)
        {
          std::vector<std::string> eventNames{};
          for (auto& event : anm2.content.events | std::views::values)
            eventNames.emplace_back(event.name);

          imgui::combo_strings("Event", frame ? &frame->eventID : &dummy_value<int>(), eventNames);
          ImGui::SetItemTooltip("%s", "Change the event this trigger uses.");
          ImGui::InputInt("At Frame", frame ? &frame->atFrame : &dummy_value<int>(), step::NORMAL, step::FAST,
                          !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0);
          ImGui::SetItemTooltip("%s", "Change the frame the trigger will be activated at.");
        }
        else
        {
          ImGui::BeginDisabled(type == anm2::ROOT || type == anm2::NULL_);
          {
            if (ImGui::InputFloat2("Crop", frame ? value_ptr(useFrame.crop) : &dummy_value<float>(),
                                   frame ? vec2_format_get(useFrame.crop) : ""))
              document.frame_crop_set(frame, useFrame.crop);
            ImGui::SetItemTooltip("%s", "Change the crop position the frame uses.");

            if (ImGui::InputFloat2("Size", frame ? value_ptr(useFrame.size) : &dummy_value<float>(),
                                   frame ? vec2_format_get(useFrame.size) : ""))
              document.frame_size_set(frame, useFrame.size);
            ImGui::SetItemTooltip("%s", "Change the size of the crop the frame uses.");
          }
          ImGui::EndDisabled();

          if (ImGui::InputFloat2("Position", frame ? value_ptr(useFrame.position) : &dummy_value<float>(),
                                 frame ? vec2_format_get(useFrame.position) : ""))
            document.frame_position_set(frame, useFrame.position);
          ImGui::SetItemTooltip("%s", "Change the position of the frame.");

          ImGui::BeginDisabled(type == anm2::ROOT || type == anm2::NULL_);
          {
            if (ImGui::InputFloat2("Pivot", frame ? value_ptr(useFrame.pivot) : &dummy_value<float>(),
                                   frame ? vec2_format_get(useFrame.pivot) : ""))
              document.frame_pivot_set(frame, useFrame.pivot);
            ImGui::SetItemTooltip("%s", "Change the pivot of the frame; i.e., where it is centered.");
          }
          ImGui::EndDisabled();

          if (ImGui::InputFloat2("Scale", frame ? value_ptr(useFrame.scale) : &dummy_value<float>(),
                                 frame ? vec2_format_get(useFrame.scale) : ""))
            document.frame_scale_set(frame, useFrame.scale);
          ImGui::SetItemTooltip("%s", "Change the scale of the frame, in percent.");

          if (ImGui::InputFloat("Rotation", frame ? &useFrame.rotation : &dummy_value<float>(), step::NORMAL,
                                step::FAST, frame ? float_format_get(useFrame.rotation) : ""))
            document.frame_rotation_set(frame, useFrame.rotation);
          ImGui::SetItemTooltip("%s", "Change the rotation of the frame.");

          if (ImGui::InputInt("Duration", frame ? &useFrame.delay : &dummy_value<int>(), step::NORMAL, step::FAST,
                              !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0))
            document.frame_delay_set(frame, useFrame.delay);
          ImGui::SetItemTooltip("%s", "Change how long the frame lasts.");

          if (ImGui::ColorEdit4("Tint", frame ? value_ptr(useFrame.tint) : &dummy_value<float>()))
            document.frame_tint_set(frame, useFrame.tint);
          ImGui::SetItemTooltip("%s", "Change the tint of the frame.");

          if (ImGui::ColorEdit3("Color Offset", frame ? value_ptr(useFrame.colorOffset) : &dummy_value<float>()))
            document.frame_color_offset_set(frame, useFrame.colorOffset);
          ImGui::SetItemTooltip("%s", "Change the color added onto the frame.");

          if (ImGui::Checkbox("Visible", frame ? &useFrame.isVisible : &dummy_value<bool>()))
            document.frame_is_visible_set(frame, useFrame.isVisible);
          ImGui::SetItemTooltip("%s", "Toggle the frame's visibility.");

          ImGui::SameLine();

          if (ImGui::Checkbox("Interpolated", frame ? &useFrame.isInterpolated : &dummy_value<bool>()))
            document.frame_is_interpolated_set(frame, useFrame.isInterpolated);
          ImGui::SetItemTooltip(
              "%s", "Toggle the frame interpolating; i.e., blending its values into the next frame based on the time.");

          auto widgetSize = imgui::widget_size_with_row_get(2);

          if (ImGui::Button("Flip X", widgetSize)) document.frame_flip_x(frame);
          ImGui::SetItemTooltip("%s", "Flip the horizontal scale of the frame, to cheat mirroring the frame "
                                      "horizontally.\n(Note: the format does not support mirroring.)");
          ImGui::SameLine();
          if (ImGui::Button("Flip Y", widgetSize)) document.frame_flip_y(frame);
          ImGui::SetItemTooltip("%s", "Flip the vertical scale of the frame, to cheat mirroring the frame "
                                      "vertically.\n(Note: the format does not support mirroring.)");
        }
      }
      ImGui::EndDisabled();
    }
    ImGui::End();
  }
}