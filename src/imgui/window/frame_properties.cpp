#include "frame_properties.h"

#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <ranges>

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
            if (combo_id_mapped(localize.get(BASIC_EVENT), frame ? &useFrame.eventID : &dummy_value_negative<int>(),
                                document.event.ids, document.event.labels) &&
                frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_EVENT), Document::FRAMES,
                            frame->eventID = useFrame.eventID);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_EVENT));

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

            ImGui::SeparatorText(localize.get(LABEL_SOUNDS));

            auto childSize = imgui::size_without_footer_get();

            if (ImGui::BeginChild("##Sounds Child", childSize, ImGuiChildFlags_Borders))
            {
              if (!useFrame.soundIDs.empty())
              {
                for (auto [i, id] : std::views::enumerate(useFrame.soundIDs))
                {
                  ImGui::PushID(i);
                  if (combo_id_mapped("##Sound", frame ? &id : &dummy_value_negative<int>(), document.sound.ids,
                                      document.sound.labels) &&
                      frame)
                    DOCUMENT_EDIT(document, localize.get(EDIT_TRIGGER_SOUND), Document::FRAMES,
                                  frame->soundIDs[i] = id);
                  ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_SOUND));
                  ImGui::PopID();
                }
              }
            }
            ImGui::EndChild();

            auto widgetSize = imgui::widget_size_with_row_get(2);

            if (ImGui::Button(localize.get(BASIC_ADD), widgetSize) && frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_ADD_TRIGGER_SOUND), Document::FRAMES,
                            frame->soundIDs.push_back(-1));
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADD_TRIGGER_SOUND));

            ImGui::SameLine();

            ImGui::BeginDisabled(useFrame.soundIDs.empty());
            if (ImGui::Button(localize.get(BASIC_REMOVE), widgetSize) && frame)
              DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_TRIGGER_SOUND), Document::FRAMES,
                            frame->soundIDs.pop_back());
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REMOVE_TRIGGER_SOUND));
            ImGui::EndDisabled();
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
            {
              if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
              {
                DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_FLIP_X), Document::FRAMES,
                              frame->scale.x = -frame->scale.x;
                              frame->position.x = -frame->position.x);
              }
              else
              {
                DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_FLIP_X), Document::FRAMES,
                              frame->scale.x = -frame->scale.x);
              }
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FLIP_X));
            ImGui::SameLine();
            if (ImGui::Button(localize.get(LABEL_FLIP_Y), widgetSize) && frame)
            {
              if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
              {
                DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_FLIP_Y), Document::FRAMES,
                              frame->scale.y = -frame->scale.y;
                              frame->position.y = -frame->position.y);
              }
              else
              {
                DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_FLIP_Y), Document::FRAMES,
                              frame->scale.y = -frame->scale.y);
              }
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FLIP_Y));
          }
        }
        ImGui::EndDisabled();
      }
      else
        changeAllFrameProperties.update(document, settings);
    }
    ImGui::End();

    dummy_value_negative<int>() = -1;
    dummy_value<float>() = 0;
    dummy_value<int>() = 0;
    dummy_value<bool>() = 0;
    dummy_value<vec2>() = vec2();
    dummy_value<vec3>() = vec3();
    dummy_value<vec4>() = vec4();
  }
}
