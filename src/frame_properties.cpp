#include "frame_properties.h"

#include <ranges>

#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "math.h"
#include "types.h"

using namespace anm2ed::settings;
using namespace anm2ed::document_manager;
using namespace anm2ed::math;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::frame_properties
{

  void FrameProperties::update(DocumentManager& manager, Settings& settings)
  {
    if (ImGui::Begin("Frame Properties", &settings.windowIsFrameProperties))
    {
      auto& document = *manager.get();
      auto& anm2 = document.anm2;
      auto& reference = document.reference;
      auto& type = reference.itemType;
      auto& isRound = settings.propertiesIsRound;
      auto frame = document.frame_get();

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
            if (ImGui::InputFloat2("Crop", frame ? value_ptr(frame->crop) : &dummy_value<float>(),
                                   frame ? vec2_format_get(frame->crop) : ""))
              if (isRound) frame->crop = ivec2(frame->crop);
            ImGui::SetItemTooltip("%s", "Change the crop position the frame uses.");

            if (ImGui::InputFloat2("Size", frame ? value_ptr(frame->size) : &dummy_value<float>(),
                                   frame ? vec2_format_get(frame->size) : ""))
              if (isRound) frame->crop = ivec2(frame->size);
            ImGui::SetItemTooltip("%s", "Change the size of the crop the frame uses.");
          }
          ImGui::EndDisabled();

          if (ImGui::InputFloat2("Position", frame ? value_ptr(frame->position) : &dummy_value<float>(),
                                 frame ? vec2_format_get(frame->position) : ""))
            if (isRound) frame->position = ivec2(frame->position);
          ImGui::SetItemTooltip("%s", "Change the position of the frame.");

          ImGui::BeginDisabled(type == anm2::ROOT || type == anm2::NULL_);
          {
            if (ImGui::InputFloat2("Pivot", frame ? value_ptr(frame->pivot) : &dummy_value<float>(),
                                   frame ? vec2_format_get(frame->pivot) : ""))
              if (isRound) frame->position = ivec2(frame->position);
            ImGui::SetItemTooltip("%s", "Change the pivot of the frame; i.e., where it is centered.");
          }
          ImGui::EndDisabled();

          if (ImGui::InputFloat2("Scale", frame ? value_ptr(frame->scale) : &dummy_value<float>(),
                                 frame ? vec2_format_get(frame->scale) : ""))
            if (isRound) frame->position = ivec2(frame->position);
          ImGui::SetItemTooltip("%s", "Change the scale of the frame, in percent.");

          if (ImGui::InputFloat("Rotation", frame ? &frame->rotation : &dummy_value<float>(), step::NORMAL, step::FAST,
                                frame ? float_format_get(frame->rotation) : ""))
            if (isRound) frame->rotation = (int)frame->rotation;
          ImGui::SetItemTooltip("%s", "Change the rotation of the frame.");

          ImGui::InputInt("Duration", frame ? &frame->delay : &dummy_value<int>(), step::NORMAL, step::FAST,
                          !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0);
          ImGui::SetItemTooltip("%s", "Change how long the frame lasts.");

          ImGui::ColorEdit4("Tint", frame ? value_ptr(frame->tint) : &dummy_value<float>());
          ImGui::SetItemTooltip("%s", "Change the tint of the frame.");
          ImGui::ColorEdit3("Color Offset", frame ? value_ptr(frame->offset) : &dummy_value<float>());
          ImGui::SetItemTooltip("%s", "Change the color added onto the frame.");

          ImGui::Checkbox("Visible", frame ? &frame->isVisible : &dummy_value<bool>());
          ImGui::SetItemTooltip("%s", "Toggle the frame's visibility.");
          ImGui::SameLine();
          ImGui::Checkbox("Interpolated", frame ? &frame->isInterpolated : &dummy_value<bool>());
          ImGui::SetItemTooltip(
              "%s", "Toggle the frame interpolating; i.e., blending its values into the next frame based on the time.");

          ImGui::SameLine();
          ImGui::EndDisabled();
          ImGui::Checkbox("Round", &settings.propertiesIsRound);
          ImGui::BeginDisabled(!frame);
          ImGui::SetItemTooltip(
              "%s", "When toggled, decimal values will be snapped to their nearest whole value when changed.");

          auto widgetSize = imgui::widget_size_with_row_get(2);

          if (ImGui::Button("Flip X", widgetSize))
            if (frame) frame->scale.x = -frame->scale.x;
          ImGui::SetItemTooltip("%s", "Flip the horizontal scale of the frame, to cheat mirroring the frame "
                                      "horizontally.\n(Note: the format does not support mirroring.)");
          ImGui::SameLine();
          if (ImGui::Button("Flip Y", widgetSize))
            if (frame) frame->scale.y = -frame->scale.y;
          ImGui::SetItemTooltip("%s", "Flip the vertical scale of the frame, to cheat mirroring the frame "
                                      "vertically.\n(Note: the format does not support mirroring.)");
        }
      }
      ImGui::EndDisabled();
    }
    ImGui::End();
  }
}