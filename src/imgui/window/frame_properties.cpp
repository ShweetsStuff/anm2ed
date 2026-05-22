#include "frame_properties.hpp"

#include <limits>
#include <ranges>
#include <string>
#include <vector>

#include "math.hpp"
#include "strings.hpp"
#include "types.hpp"
#include "util/imgui/imgui.hpp"

using namespace anm2ed::util::math;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::imgui
{
  void FrameProperties::update(Manager& manager, Settings& settings)
  {
    auto& document = *manager.get();
    auto frameSelectionCount =
        document.frames.references.empty() ? document.frames.selection.size() : document.frames.references.size();
    auto isSingleFrameSelection = frameSelectionCount == 1;
    auto isMultiFrameSelection = frameSelectionCount > 1;
    auto isSingleFrameBatchMode =
        isSingleFrameSelection && document.reference.itemType != TRIGGER && isBatchMode;
    auto isBatchFrameProperties = isMultiFrameSelection || isSingleFrameBatchMode;
    auto windowLabel = std::string(localize.get(isBatchFrameProperties ? LABEL_CHANGE_ALL_FRAME_PROPERTIES
                                                                       : LABEL_FRAME_PROPERTIES_WINDOW));
    if (isBatchFrameProperties) windowLabel += "###Frame Properties";

    if (ImGui::Begin(windowLabel.c_str(), &settings.windowIsFrameProperties))
    {
      auto& anm2 = document.anm2;
      auto& reference = document.reference;
      auto& type = reference.itemType;
      auto itemType = static_cast<ItemType>(type);
      auto frame = anm2.element_get(reference.animationIndex, itemType, reference.frameIndex, reference.itemID);

      auto frame_edit = [&](auto message, auto behavior)
      {
        auto queuedReference = reference;
        manager.command_push({manager.selected,
                              [=](Manager&, Document& document) mutable
                              {
                                auto itemType = static_cast<ItemType>(queuedReference.itemType);
                                auto frame = document.anm2.element_get(queuedReference.animationIndex, itemType,
                                                                       queuedReference.frameIndex,
                                                                       queuedReference.itemID);
                                auto item =
                                    document.anm2.element_get(queuedReference.animationIndex, itemType,
                                                              queuedReference.itemID);
                                if (!frame) return;

                                document.snapshot(localize.get(message));
                                behavior(document, *frame, item, queuedReference);
                                document.anm2_change(Document::FRAMES);
                              }});
      };

      auto persistent_edit = [&](auto state, auto message, auto behavior)
      {
        if (state == edit::NONE) return;

        auto queuedReference = reference;
        manager.command_push({manager.selected,
                              [=](Manager&, Document& document) mutable
                              {
                                auto itemType = static_cast<ItemType>(queuedReference.itemType);
                                auto frame = document.anm2.element_get(queuedReference.animationIndex, itemType,
                                                                       queuedReference.frameIndex,
                                                                       queuedReference.itemID);
                                auto item =
                                    document.anm2.element_get(queuedReference.animationIndex, itemType,
                                                              queuedReference.itemID);
                                if (!frame) return;

                                if (state == edit::START) document.snapshot(localize.get(message));
                                behavior(document, *frame, item, queuedReference);
                                if (state == edit::END) document.anm2_change(Document::FRAMES);
                              }});
      };

      auto regionLabelsString = std::vector<std::string>{localize.get(BASIC_NONE)};
      auto regionLabels = std::vector<const char*>{regionLabelsString[0].c_str()};
      auto regionIds = std::vector<int>{-1};
      auto interpolationLabelsString =
          std::vector<std::string>{localize.get(BASIC_NONE), localize.get(BASIC_LINEAR), localize.get(BASIC_EASE_IN),
                                   localize.get(BASIC_EASE_OUT), localize.get(BASIC_EASE_IN_OUT)};
      auto interpolationLabels =
          std::vector<const char*>{interpolationLabelsString[0].c_str(), interpolationLabelsString[1].c_str(),
                                   interpolationLabelsString[2].c_str(), interpolationLabelsString[3].c_str(),
                                   interpolationLabelsString[4].c_str()};
      auto interpolationValues =
          std::vector<int>{(int)Interpolation::NONE, (int)Interpolation::LINEAR, (int)Interpolation::EASE_IN,
                           (int)Interpolation::EASE_OUT, (int)Interpolation::EASE_IN_OUT};

      if (type == LAYER && reference.itemID != -1)
      {
        if (auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, reference.itemID))
        {
          auto regionIt = document.regionBySpritesheet.find(layer->spritesheetId);
          if (regionIt != document.regionBySpritesheet.end() && !regionIt->second.ids.empty() &&
              !regionIt->second.labels.empty())
          {
            regionLabels = regionIt->second.labels;
            regionIds = regionIt->second.ids;
          }
        }
      }

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
        auto useFrame = frame ? *frame : Element();
        auto displayFrame = frame && type == LAYER && reference.itemID != -1
                                ? anm2.frame_effective(reference.itemID, *frame)
                                : useFrame;

        ImGui::BeginDisabled(!frame);
        {
          if (type == TRIGGER)
          {
            if (combo_id_mapped(localize.get(BASIC_EVENT), frame ? &useFrame.eventId : &dummy_value_negative<int>(),
                                document.event.ids, document.event.labels) &&
                frame)
            {
              auto eventId = useFrame.eventId;
              frame_edit(EDIT_TRIGGER_EVENT,
                         [eventId](Document&, Element& frame, Element*, const Reference&) { frame.eventId = eventId; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_EVENT));

            if (input_int_range(localize.get(BASIC_AT_FRAME), frame ? useFrame.atFrame : dummy_value<int>(), 0,
                                std::numeric_limits<int>::max(), STEP, STEP_FAST,
                                !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0) &&
                frame)
            {
              auto atFrame = useFrame.atFrame;
              frame_edit(EDIT_TRIGGER_AT_FRAME,
                         [atFrame](Document& document, Element& frame, Element* item, const Reference&)
                         {
                           frame.atFrame = atFrame;
                           if (!item) return;
                           frames_sort_by_at_frame(*item);
                           document.reference.frameIndex = frame_index_from_at_frame_get(*item, atFrame);
                           document.frames.selection = {document.reference.frameIndex};
                           document.frames.references = {document.reference};
                         });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIGGER_AT_FRAME));

            if (ImGui::Checkbox(localize.get(BASIC_VISIBLE), frame ? &useFrame.isVisible : &dummy_value<bool>()) &&
                frame)
            {
              auto isVisible = useFrame.isVisible;
              frame_edit(EDIT_TRIGGER_VISIBILITY,
                         [isVisible](Document&, Element& frame, Element*, const Reference&)
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
                  if (combo_id_mapped("##Sound", frame ? &id : &dummy_value_negative<int>(), document.sound.ids,
                                      document.sound.labels) &&
                      frame)
                  {
                    auto soundIndex = (std::size_t)i;
                    auto soundId = id;
                    frame_edit(EDIT_TRIGGER_SOUND,
                               [soundIndex, soundId](Document&, Element& frame, Element*, const Reference&)
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
              frame_edit(EDIT_ADD_TRIGGER_SOUND,
                         [](Document&, Element& frame, Element*, const Reference&) { frame.soundIds.push_back(-1); });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADD_TRIGGER_SOUND));

            ImGui::SameLine();

            ImGui::BeginDisabled(useFrame.soundIds.empty());
            if (ImGui::Button(localize.get(BASIC_REMOVE), widgetSize) && frame)
              frame_edit(EDIT_REMOVE_TRIGGER_SOUND,
                         [](Document&, Element& frame, Element*, const Reference&)
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
            ImGui::BeginDisabled(type == ROOT || type == NULL_ || isRegionSet);
            {
              auto cropDisplay = frame ? displayFrame.crop : vec2();
              auto cropEdit =
                  drag_float2_persistent(localize.get(BASIC_CROP), frame ? &cropDisplay : &dummy_value<vec2>(),
                                         DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(displayFrame.crop) : "");
              persistent_edit(cropEdit, EDIT_FRAME_CROP,
                              [cropDisplay](Document&, Element& frame, Element*, const Reference&)
                              { frame.crop = cropDisplay; });
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CROP));

              auto sizeDisplay = frame ? displayFrame.size : vec2();
              auto sizeEdit =
                  drag_float2_persistent(localize.get(BASIC_SIZE), frame ? &sizeDisplay : &dummy_value<vec2>(),
                                         DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(displayFrame.size) : "");
              persistent_edit(sizeEdit, EDIT_FRAME_SIZE,
                              [sizeDisplay](Document&, Element& frame, Element*, const Reference&)
                              { frame.size = sizeDisplay; });
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SIZE));
            }
            ImGui::EndDisabled();

            auto positionEdit =
                drag_float2_persistent(localize.get(BASIC_POSITION),
                                       frame ? &useFrame.position : &dummy_value<vec2>(),
                                       DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(frame->position) : "");
            persistent_edit(positionEdit, EDIT_FRAME_POSITION,
                            [position = useFrame.position](Document&, Element& frame, Element*, const Reference&)
                            { frame.position = position; });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_POSITION));

            ImGui::BeginDisabled(type == ROOT || type == NULL_ || isRegionSet);
            {
              auto pivotDisplay = frame ? displayFrame.pivot : vec2();
              auto pivotEdit =
                  drag_float2_persistent(localize.get(BASIC_PIVOT), frame ? &pivotDisplay : &dummy_value<vec2>(),
                                         DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(displayFrame.pivot) : "");
              persistent_edit(pivotEdit, EDIT_FRAME_PIVOT,
                              [pivotDisplay](Document&, Element& frame, Element*, const Reference&)
                              { frame.pivot = pivotDisplay; });
              ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PIVOT));
            }
            ImGui::EndDisabled();

            auto scaleEdit =
                drag_float2_persistent(localize.get(BASIC_SCALE), frame ? &useFrame.scale : &dummy_value<vec2>(),
                                       DRAG_SPEED, 0.0f, 0.0f, frame ? vec2_format_get(frame->scale) : "");
            persistent_edit(scaleEdit, EDIT_FRAME_SCALE,
                            [scale = useFrame.scale](Document&, Element& frame, Element*, const Reference&)
                            { frame.scale = scale; });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SCALE));

            auto rotationEdit =
                drag_float_persistent(localize.get(BASIC_ROTATION), frame ? &useFrame.rotation : &dummy_value<float>(),
                                      DRAG_SPEED, 0.0f, 0.0f, frame ? float_format_get(frame->rotation) : "");
            persistent_edit(rotationEdit, EDIT_FRAME_ROTATION,
                            [rotation = useFrame.rotation](Document&, Element& frame, Element*, const Reference&)
                            { frame.rotation = rotation; });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROTATION));

            if (input_int_range(localize.get(BASIC_DURATION), frame ? useFrame.duration : dummy_value<int>(),
                                frame ? FRAME_DURATION_MIN : 0, FRAME_DURATION_MAX, STEP, STEP_FAST,
                                !frame ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0) &&
                frame)
            {
              auto duration = useFrame.duration;
              frame_edit(EDIT_FRAME_DURATION,
                         [duration](Document&, Element& frame, Element*, const Reference&) { frame.duration = duration; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_DURATION));

            auto tintEdit =
                color_edit4_persistent(localize.get(BASIC_TINT), frame ? &useFrame.tint : &dummy_value<vec4>());
            persistent_edit(tintEdit, EDIT_FRAME_TINT,
                            [tint = useFrame.tint](Document&, Element& frame, Element*, const Reference&)
                            { frame.tint = tint; });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TINT));

            auto colorOffsetEdit = color_edit3_persistent(localize.get(BASIC_COLOR_OFFSET),
                                                          frame ? &useFrame.colorOffset : &dummy_value<vec3>());
            persistent_edit(colorOffsetEdit, EDIT_FRAME_COLOR_OFFSET,
                            [colorOffset = useFrame.colorOffset](Document&, Element& frame, Element*, const Reference&)
                            { frame.colorOffset = colorOffset; });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_COLOR_OFFSET));

            ImGui::BeginDisabled(type != LAYER);
            if (combo_id_mapped(localize.get(BASIC_REGION), frame ? &useFrame.regionId : &dummy_value_negative<int>(),
                regionIds, regionLabels) &&
                frame)
            {
              auto regionId = useFrame.regionId;
              frame_edit(EDIT_SET_REGION_PROPERTIES,
                         [regionId](Document& document, Element& frame, Element*, const Reference& reference)
                         {
                           frame.regionId = regionId;
                           auto effectiveFrame = document.anm2.frame_effective(reference.itemID, frame);
                           frame.crop = effectiveFrame.crop;
                           frame.size = effectiveFrame.size;
                           frame.pivot = effectiveFrame.pivot;
                         });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REGION));
            ImGui::EndDisabled();

            auto interpolationValue = frame ? static_cast<int>(useFrame.interpolation) : dummy_value<int>();
            if (combo_id_mapped(localize.get(BASIC_INTERPOLATED), &interpolationValue, interpolationValues,
                                interpolationLabels) &&
                frame)
              frame_edit(EDIT_FRAME_INTERPOLATION,
                         [interpolationValue](Document&, Element& frame, Element*, const Reference&)
                         { frame.interpolation = static_cast<Interpolation>(interpolationValue); });
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FRAME_INTERPOLATION));

            if (ImGui::Checkbox(localize.get(BASIC_VISIBLE), frame ? &useFrame.isVisible : &dummy_value<bool>()) &&
                frame)
            {
              auto isVisible = useFrame.isVisible;
              frame_edit(EDIT_FRAME_VISIBILITY,
                         [isVisible](Document&, Element& frame, Element*, const Reference&)
                         { frame.isVisible = isVisible; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FRAME_VISIBILITY));

            auto widgetSize = widget_size_with_row_get(2);

            if (ImGui::Button(localize.get(LABEL_FLIP_X), widgetSize) && frame)
            {
              if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
                frame_edit(EDIT_FRAME_FLIP_X,
                           [](Document&, Element& frame, Element*, const Reference&)
                           {
                             frame.scale.x = -frame.scale.x;
                             frame.position.x = -frame.position.x;
                           });
              else
                frame_edit(EDIT_FRAME_FLIP_X,
                           [](Document&, Element& frame, Element*, const Reference&) { frame.scale.x = -frame.scale.x; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FLIP_X));
            ImGui::SameLine();
            if (ImGui::Button(localize.get(LABEL_FLIP_Y), widgetSize) && frame)
            {
              if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
                frame_edit(EDIT_FRAME_FLIP_Y,
                           [](Document&, Element& frame, Element*, const Reference&)
                           {
                             frame.scale.y = -frame.scale.y;
                             frame.position.y = -frame.position.y;
                           });
              else
                frame_edit(EDIT_FRAME_FLIP_Y,
                           [](Document&, Element& frame, Element*, const Reference&) { frame.scale.y = -frame.scale.y; });
            }
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FLIP_Y));
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
