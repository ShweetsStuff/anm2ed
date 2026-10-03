#include "panel.hpp"

#include <limits>
#include <ranges>
#include <string>
#include <vector>

#include "math.hpp"
#include "model/frames.hpp"
#include "strings.hpp"
#include "types.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"

using namespace anm2ed::util::math;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::imgui
{
  struct Vec2Field
  {
    StringType label;
    StringType tooltip;
    StringType edit;
    vec2 model::Frame::* member;
    bool isRegionField;
  };

  constexpr Vec2Field VEC2_FIELDS[] = {
      {BASIC_CROP, TOOLTIP_CROP, EDIT_FRAME_CROP, &model::Frame::crop, true},
      {BASIC_SIZE, TOOLTIP_SIZE, EDIT_FRAME_SIZE, &model::Frame::size, true},
      {BASIC_POSITION, TOOLTIP_POSITION, EDIT_FRAME_POSITION, &model::Frame::position, false},
      {BASIC_PIVOT, TOOLTIP_PIVOT, EDIT_FRAME_PIVOT, &model::Frame::pivot, true},
      {BASIC_SCALE, TOOLTIP_SCALE, EDIT_FRAME_SCALE, &model::Frame::scale, false},
      {BASIC_SHEAR, TOOLTIP_SHEAR, EDIT_FRAME_SHEAR, &model::Frame::shear, false}};

  struct FlipButton
  {
    StringType label;
    StringType edit;
    StringType tooltip;
  };

  constexpr FlipButton FLIP_BUTTONS[] = {{LABEL_FLIP_X, EDIT_FRAME_FLIP_X, TOOLTIP_FLIP_X},
                                         {LABEL_FLIP_Y, EDIT_FRAME_FLIP_Y, TOOLTIP_FLIP_Y}};

  void frame_properties_update(Manager& manager, Settings& settings, FramePropertiesPanel& panel)
  {
    auto& [changeAllFrameProperties, isBatchMode] = panel;
    auto& document = *manager.get();
    auto frameSelectionCount = document.frame_references_get(Document::FrameReferenceFallback::NONE).size();
    if (frameSelectionCount == 0 && document.reference_get().frameIndex >= 0) frameSelectionCount = 1;
    auto isSingleFrameSelection = frameSelectionCount == 1;
    auto isMultiFrameSelection = frameSelectionCount > 1;
    auto isSingleFrameBatchMode = isSingleFrameSelection && document.reference_get().itemType != TRIGGER && isBatchMode;
    auto isBatchFrameProperties = isMultiFrameSelection || isSingleFrameBatchMode;
    auto windowLabel = std::string(
        localize.get(isBatchFrameProperties ? LABEL_CHANGE_ALL_FRAME_PROPERTIES : LABEL_FRAME_PROPERTIES_WINDOW));
    if (isBatchFrameProperties) windowLabel += "###Frame Properties";

    if (ImGui::Begin(windowLabel.c_str(), &settings.windowIsFrameProperties))
    {
      auto& model = document.model;
      auto reference = document.reference_get();
      auto& type = reference.itemType;
      auto frame = model.frame_get(reference);

      auto frame_edit = [&](edit_state::Type state, StringType message, auto behavior)
      {
        if (state == edit_state::NONE) return;
        manager.command_push({manager.selected, [=, queuedReference = reference](Manager&, Document& document) mutable
                              {
                                if (!document.model.frame_get(queuedReference)) return;
                                if (state == edit_state::START || state == edit_state::COMPLETE)
                                  document.edit_begin(message);
                                auto item = document.model.track_edit(queuedReference);
                                behavior(document, item->frames[queuedReference.frameIndex], item, queuedReference);
                                if (state == edit_state::END || state == edit_state::COMPLETE) document.change();
                              }});
      };

      auto regions = document.choices_get(SelectionKind::REGIONS, type == LAYER ? reference.itemID : -1);
      auto events = document.choices_get(SelectionKind::EVENTS);
      auto sounds = document.choices_get(SelectionKind::SOUNDS);
      auto shaders = document.choices_get(SelectionKind::SHADERS);
      std::vector<const char*> interpolationLabels{};
      for (auto label : INTERPOLATION_LABELS)
        interpolationLabels.push_back(localize.get(label));

      auto mode_selector_draw = [&]()
      {
        if (!isSingleFrameSelection || type == TRIGGER) return;
        ImGui::SeparatorText(localize.get(BASIC_MODE));
        int mode = isBatchMode ? 1 : 0;
        ImGui::RadioButton(localize.get(BASIC_SINGLE), &mode, 0);
        ImGui::SameLine();
        ImGui::RadioButton(localize.get(BASIC_BATCH), &mode, 1);
        isBatchMode = mode == 1;
      };

      if (isSingleFrameBatchMode)
      {
        changeAllFrameProperties.update(manager, document, settings);
        mode_selector_draw();
      }
      else if (!isMultiFrameSelection)
      {
        auto useFrame = frame ? *frame : model::Frame();
        auto displayFrame = frame && type == LAYER && reference.itemID != -1
                                ? model.frame_effective(reference.itemID, *frame)
                                : useFrame;

        ImGui::BeginDisabled(!frame);
        {
          if (type == TRIGGER)
          {
            if (combo_id_mapped(localize.get(BASIC_EVENT), frame ? &useFrame.eventId : &dummy_value_negative<int>(),
                                events.ids, events.labels) &&
                frame)
            {
              auto eventId = useFrame.eventId;
              frame_edit(edit_state::COMPLETE, EDIT_TRIGGER_EVENT,
                         [eventId](Document&, model::Frame& frame, model::Track*, const Reference&)
                         { frame.eventId = eventId; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_EVENT));

            if (input_int_range(localize.get(BASIC_AT_FRAME), frame ? useFrame.atFrame : dummy_value<int>(), 0,
                                std::numeric_limits<int>::max(), STEP, STEP_FAST,
                                !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0) &&
                frame)
            {
              auto atFrame = useFrame.atFrame;
              frame_edit(edit_state::COMPLETE, EDIT_TRIGGER_AT_FRAME,
                         [atFrame](Document& document, model::Frame& frame, model::Track* item, const Reference&)
                         {
                           frame.atFrame = atFrame;
                           if (!item) return;
                           model::frames_sort_by_at_frame(*item);
                           auto reference = document.reference_get();
                           reference.frameIndex = model::frame_index_from_at_frame_get(*item, atFrame);
                           document.reference_set(reference);
                           document.frame_references_set({reference});
                         });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_AT_FRAME));

            if (ImGui::Checkbox(localize.get(BASIC_VISIBLE), frame ? &useFrame.isVisible : &dummy_value<bool>()) &&
                frame)
            {
              auto isVisible = useFrame.isVisible;
              frame_edit(edit_state::COMPLETE, EDIT_TRIGGER_VISIBILITY,
                         [isVisible](Document&, model::Frame& frame, model::Track*, const Reference&)
                         { frame.isVisible = isVisible; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_VISIBILITY));

            ImGui::SeparatorText(localize.get(LABEL_SOUNDS));

            auto childSize = imgui::size_without_footer_get();

            if (ImGui::BeginChild("##Sounds Child", childSize, ImGuiChildFlags_Borders))
            {
              if (!useFrame.soundIds.empty())
              {
                for (auto [i, id] : std::views::enumerate(useFrame.soundIds))
                {
                  ImGui::PushID(i);
                  if (combo_id_mapped("##Sound", frame ? &id : &dummy_value_negative<int>(), sounds.ids,
                                      sounds.labels) &&
                      frame)
                  {
                    auto soundIndex = (std::size_t)i;
                    auto soundId = id;
                    frame_edit(edit_state::COMPLETE, EDIT_TRIGGER_SOUND,
                               [soundIndex, soundId](Document&, model::Frame& frame, model::Track*, const Reference&)
                               {
                                 if (soundIndex < frame.soundIds.size()) frame.soundIds[soundIndex] = soundId;
                               });
                  }
                  ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_SOUND));
                  ImGui::PopID();
                }
              }
            }
            ImGui::EndChild();

            auto widgetSize = imgui::widget_size_with_row_get(2);

            if (ImGui::Button(localize.get(BASIC_ADD), widgetSize) && frame)
              frame_edit(edit_state::COMPLETE, EDIT_ADD_TRIGGER_SOUND,
                         [](Document&, model::Frame& frame, model::Track*, const Reference&)
                         { frame.soundIds.push_back(-1); });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADD_TRIGGER_SOUND));

            ImGui::SameLine();

            ImGui::BeginDisabled(useFrame.soundIds.empty());
            if (ImGui::Button(localize.get(BASIC_REMOVE), widgetSize) && frame)
              frame_edit(edit_state::COMPLETE, EDIT_REMOVE_TRIGGER_SOUND,
                         [](Document&, model::Frame& frame, model::Track*, const Reference&)
                         {
                           if (!frame.soundIds.empty()) frame.soundIds.pop_back();
                         });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REMOVE_TRIGGER_SOUND));
            ImGui::EndDisabled();
          }
          else
          {
            bool isRegionSet = frame && displayFrame.regionId != -1 && displayFrame.crop == frame->crop &&
                               displayFrame.size == frame->size && displayFrame.pivot == frame->pivot;
            for (auto& field : VEC2_FIELDS)
            {
              const auto& source = field.isRegionField ? displayFrame : useFrame;
              auto value = frame ? source.*field.member : vec2();
              ImGui::BeginDisabled(field.isRegionField && (type == ROOT || type == NULL_ || isRegionSet));
              auto state =
                  drag_float2_persistent(localize.get(field.label), frame ? &value : &dummy_value<vec2>(), DRAG_SPEED,
                                         0.0f, 0.0f, frame ? vec2_format_get(source.*field.member) : "");
              frame_edit(state, field.edit,
                         [value, member = field.member](Document&, model::Frame& frame, model::Track*, const Reference&)
                         { frame.*member = value; });
              ImGui::SetItemTooltip("%s", localize.get(field.tooltip));
              ImGui::EndDisabled();
            }

            auto rotationEdit =
                drag_float_persistent(localize.get(BASIC_ROTATION), frame ? &useFrame.rotation : &dummy_value<float>(),
                                      DRAG_SPEED, 0.0f, 0.0f, frame ? float_format_get(frame->rotation) : "");
            frame_edit(rotationEdit, EDIT_FRAME_ROTATION,
                       [rotation = useFrame.rotation](Document&, model::Frame& frame, model::Track*, const Reference&)
                       { frame.rotation = rotation; });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROTATION));

            if (input_int_range(localize.get(BASIC_DURATION), frame ? useFrame.duration : dummy_value<int>(),
                                frame ? FRAME_DURATION_MIN : 0, FRAME_DURATION_MAX, STEP, STEP_FAST,
                                !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0) &&
                frame)
            {
              auto duration = useFrame.duration;
              frame_edit(edit_state::COMPLETE, EDIT_FRAME_DURATION,
                         [duration](Document&, model::Frame& frame, model::Track*, const Reference&)
                         { frame.duration = duration; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_DURATION));

            auto tintEdit =
                color_edit4_persistent(localize.get(BASIC_TINT), frame ? &useFrame.tint : &dummy_value<vec4>());
            frame_edit(tintEdit, EDIT_FRAME_TINT,
                       [tint = useFrame.tint](Document&, model::Frame& frame, model::Track*, const Reference&)
                       { frame.tint = tint; });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TINT));

            auto colorOffsetEdit = color_edit3_persistent(localize.get(BASIC_COLOR_OFFSET),
                                                          frame ? &useFrame.colorOffset : &dummy_value<vec3>());
            frame_edit(colorOffsetEdit, EDIT_FRAME_COLOR_OFFSET,
                       [colorOffset = useFrame.colorOffset](Document&, model::Frame& frame, model::Track*,
                                                            const Reference&) { frame.colorOffset = colorOffset; });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_COLOR_OFFSET));

            ImGui::BeginDisabled(type != LAYER);
            if (combo_id_mapped(localize.get(BASIC_REGION), frame ? &useFrame.regionId : &dummy_value_negative<int>(),
                                regions.ids, regions.labels) &&
                frame)
            {
              auto regionId = useFrame.regionId;
              frame_edit(edit_state::COMPLETE, EDIT_SET_REGION_PROPERTIES,
                         [regionId](Document& document, model::Frame& frame, model::Track*, const Reference& reference)
                         {
                           frame.regionId = regionId;
                           auto effectiveFrame = document.model.frame_effective(reference.itemID, frame);
                           frame.crop = effectiveFrame.crop;
                           frame.size = effectiveFrame.size;
                           frame.pivot = effectiveFrame.pivot;
                         });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REGION));
            ImGui::EndDisabled();

            auto interpolationValue = frame ? static_cast<int>(useFrame.interpolation) : dummy_value<int>();
            if (ImGui::Combo(localize.get(BASIC_INTERPOLATED), &interpolationValue, interpolationLabels.data(),
                             (int)interpolationLabels.size()) &&
                frame)
              frame_edit(edit_state::COMPLETE, EDIT_FRAME_INTERPOLATION,
                         [interpolationValue](Document&, model::Frame& frame, model::Track*, const Reference&)
                         { frame.interpolation = static_cast<Interpolation>(interpolationValue); });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FRAME_INTERPOLATION));

            ImGui::BeginDisabled(type != LAYER);
            if (combo_id_mapped(localize.get(BASIC_SHADER), frame ? &useFrame.shaderId : &dummy_value_negative<int>(),
                                shaders.ids, shaders.labels) &&
                frame)
            {
              auto shaderId = useFrame.shaderId;
              frame_edit(edit_state::COMPLETE, EDIT_FRAME_SHADER,
                         [shaderId](Document&, model::Frame& frame, model::Track*, const Reference&)
                         { frame.shaderId = shaderId; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FRAME_SHADER));
            ImGui::EndDisabled();

            if (ImGui::Checkbox(localize.get(BASIC_VISIBLE), frame ? &useFrame.isVisible : &dummy_value<bool>()) &&
                frame)
            {
              auto isVisible = useFrame.isVisible;
              frame_edit(edit_state::COMPLETE, EDIT_FRAME_VISIBILITY,
                         [isVisible](Document&, model::Frame& frame, model::Track*, const Reference&)
                         { frame.isVisible = isVisible; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FRAME_VISIBILITY));

            auto widgetSize = widget_size_with_row_get(2);

            for (int axis = 0; axis < (int)std::size(FLIP_BUTTONS); ++axis)
            {
              if (axis > 0) ImGui::SameLine();
              if (ImGui::Button(localize.get(FLIP_BUTTONS[axis].label), widgetSize) && frame)
                frame_edit(edit_state::COMPLETE, FLIP_BUTTONS[axis].edit,
                           [axis, isPositionFlipped = ImGui::IsKeyDown(ImGuiMod_Ctrl)](Document&, model::Frame& frame,
                                                                                       model::Track*, const Reference&)
                           {
                             frame.scale[axis] = -frame.scale[axis];
                             if (isPositionFlipped) frame.position[axis] = -frame.position[axis];
                           });
              ImGui::SetItemTooltip("%s", localize.get(FLIP_BUTTONS[axis].tooltip));
            }

            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight()));
          }
        }
        ImGui::EndDisabled();

        mode_selector_draw();
      }
      else
        changeAllFrameProperties.update(manager, document, settings);
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
