#include "change_all_frame_properties.hpp"

#include <algorithm>
#include <ranges>
#include <set>
#include <string>
#include <vector>

#include "math.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"

using namespace anm2ed::util::math;
using namespace glm;

namespace anm2ed::imgui::wizard
{
  constexpr int CHANGE_TYPE_COUNT = 5;
  constexpr const char* COLOR_COMPONENT_FORMATS[] = {"R:%d", "G:%d", "B:%d", "A:%d"};

  enum ChangeDestination
  {
    CHANGE_DESTINATION_FRAMES,
    CHANGE_DESTINATION_ANIMATIONS,
    CHANGE_DESTINATION_ITEMS,
  };

  struct ChangeButton
  {
    StringType label;
    StringType tooltip;
    ChangeType type;
  };

  constexpr ChangeButton CHANGE_BUTTONS[CHANGE_TYPE_COUNT] = {
      {LABEL_ADJUST, TOOLTIP_ADJUST, ChangeType::ADJUST},
      {BASIC_ADD, TOOLTIP_ADD_VALUES, ChangeType::ADD},
      {LABEL_SUBTRACT, TOOLTIP_SUBTRACT_VALUES, ChangeType::SUBTRACT},
      {LABEL_MULTIPLY, TOOLTIP_MULTIPLY_VALUES, ChangeType::MULTIPLY},
      {LABEL_DIVIDE, TOOLTIP_DIVIDE_VALUES, ChangeType::DIVIDE}};

  struct ComponentRow
  {
    StringType label;
    std::vector<bool*> isEnabled;
    float* values;
    bool isLayerOnly;
  };

  void property_toggle_begin(bool& isEnabled)
  {
    ImGui::Checkbox("##Is Enabled", &isEnabled);
    ImGui::SameLine();
    ImGui::BeginDisabled(!isEnabled);
  }

  void components_draw(const ComponentRow& row, bool isColor)
  {
    auto count = (int)row.isEnabled.size();
    auto label = localize.get(row.label);
    auto& style = ImGui::GetStyle();
    auto width = (ImGui::CalcItemWidth() - (ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.x) * (count - 1) -
                  (isColor ? ImGui::GetFrameHeight() : 0.0f)) /
                 count;

    ImGui::PushItemWidth(width);
    for (int i = 0; i < count; ++i)
    {
      ImGui::PushID(i);
      if (i > 0) ImGui::SameLine();
      property_toggle_begin(*row.isEnabled[i]);
      auto valueLabel = !isColor && i == count - 1 ? label : "##Value";
      if (isColor)
      {
        auto value = float_to_uint8(row.values[i]);
        if (ImGui::DragInt(valueLabel, &value, DRAG_SPEED, 0, 0, COLOR_COMPONENT_FORMATS[i]))
          row.values[i] = uint8_to_float(value);
      }
      else
        ImGui::DragFloat(valueLabel, &row.values[i], DRAG_SPEED, 0.0f, 0.0f, float_format_get(row.values[i]));
      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::PopItemWidth();
    if (!isColor) return;

    ImVec4 buttonColor{0.0f, 0.0f, 0.0f, 1.0f};
    for (int i = 0; i < count; ++i)
      if (*row.isEnabled[i]) (&buttonColor.x)[i] = glm::clamp(row.values[i], 0.0f, 1.0f);
    ImGui::SameLine();
    ImGui::ColorButton(label, buttonColor);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
  }

  void ChangeAllFrameProperties::update(Manager& manager, Document& document, Settings& settings, bool isFromWizard)
  {
    isChanged = false;

    auto animations = document.animations_selected_get();
    auto& destination = settings.changeDestination;
    auto& regionId = document.changeAllFramePropertiesRegionId;
    auto& shaderId = document.changeAllFramePropertiesShaderId;

    auto selectedFrameReferences = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    std::erase_if(selectedFrameReferences, [](const Reference& reference) { return reference.itemType == TRIGGER; });
    auto selectedItemReferences = document.selected_get(SelectionKind::TRACKS);
    if (auto reference = document.reference_get(); selectedItemReferences.empty() && reference.itemType != NONE)
    {
      reference.frameIndex = -1;
      selectedItemReferences.insert(reference);
    }
    std::erase_if(selectedItemReferences, [](const Reference& reference) { return reference.itemType == TRIGGER; });

    bool isSelectedFramesAvailable = !selectedFrameReferences.empty();
    bool isSelectedItemsAvailable = !selectedItemReferences.empty();
    bool isSelectedAnimationsAvailable = !animations.empty();
    if (isFromWizard)
    {
      if (destination == CHANGE_DESTINATION_FRAMES && !isSelectedFramesAvailable)
        destination = isSelectedItemsAvailable ? CHANGE_DESTINATION_ITEMS : CHANGE_DESTINATION_ANIMATIONS;
      if (destination == CHANGE_DESTINATION_ITEMS && !isSelectedItemsAvailable)
        destination = isSelectedFramesAvailable ? CHANGE_DESTINATION_FRAMES : CHANGE_DESTINATION_ANIMATIONS;
      if (destination == CHANGE_DESTINATION_ANIMATIONS && !isSelectedAnimationsAvailable && isSelectedItemsAvailable)
        destination = CHANGE_DESTINATION_ITEMS;
      if (destination == CHANGE_DESTINATION_ANIMATIONS && !isSelectedAnimationsAvailable && isSelectedFramesAvailable)
        destination = CHANGE_DESTINATION_FRAMES;
    }
    bool isFramesDestination = !isFromWizard || destination == CHANGE_DESTINATION_FRAMES;
    bool isItemsDestination = isFromWizard && destination == CHANGE_DESTINATION_ITEMS;

    auto is_layer = [](const Reference& reference) { return reference.itemType == LAYER; };
    auto isLayerPropertyAvailable = isFramesDestination  ? std::ranges::any_of(selectedFrameReferences, is_layer)
                                    : isItemsDestination ? std::ranges::any_of(selectedItemReferences, is_layer)
                                                         : settings.changeIsLayers;

    ComponentRow componentRows[] = {
        {BASIC_CROP, {&settings.changeIsCropX, &settings.changeIsCropY}, value_ptr(settings.changeCrop), true},
        {BASIC_SIZE, {&settings.changeIsSizeX, &settings.changeIsSizeY}, value_ptr(settings.changeSize), true},
        {BASIC_POSITION,
         {&settings.changeIsPositionX, &settings.changeIsPositionY},
         value_ptr(settings.changePosition),
         false},
        {BASIC_PIVOT, {&settings.changeIsPivotX, &settings.changeIsPivotY}, value_ptr(settings.changePivot), true},
        {BASIC_SCALE, {&settings.changeIsScaleX, &settings.changeIsScaleY}, value_ptr(settings.changeScale), false},
        {BASIC_SHEAR, {&settings.changeIsShearX, &settings.changeIsShearY}, value_ptr(settings.changeShear), false},
        {BASIC_ROTATION, {&settings.changeIsRotation}, &settings.changeRotation, false}};
    ComponentRow colorRows[] = {
        {BASIC_TINT,
         {&settings.changeIsTintR, &settings.changeIsTintG, &settings.changeIsTintB, &settings.changeIsTintA},
         value_ptr(settings.changeTint),
         false},
        {BASIC_COLOR_OFFSET,
         {&settings.changeIsColorOffsetR, &settings.changeIsColorOffsetG, &settings.changeIsColorOffsetB},
         value_ptr(settings.changeColorOffset),
         false}};

    auto rows_draw = [&](std::span<ComponentRow> rows, bool isColor)
    {
      for (auto& row : rows)
      {
        ImGui::PushID(row.label);
        ImGui::BeginDisabled(row.isLayerOnly && !isLayerPropertyAvailable);
        components_draw(row, isColor);
        ImGui::EndDisabled();
        ImGui::PopID();
      }
    };

    auto combo_draw = [&](const char* id, StringType label, bool& isEnabled, int& value, const std::vector<int>& ids,
                          std::vector<const char*>& labels)
    {
      ImGui::PushID(id);
      if (std::ranges::find(ids, value) == ids.end()) value = ids.front();
      property_toggle_begin(isEnabled);
      combo_id_mapped(localize.get(label), &value, ids, labels);
      ImGui::EndDisabled();
      ImGui::PopID();
    };

    auto bool_draw = [&](const char* id, StringType label, bool& isEnabled, bool& value)
    {
      ImGui::PushID(id);
      property_toggle_begin(isEnabled);
      ImGui::Checkbox(localize.get(label), &value);
      ImGui::EndDisabled();
      ImGui::PopID();
    };

    std::vector<int> noneIds{-1};
    std::string noneLabel = localize.get(BASIC_NONE);
    std::vector<const char*> noneLabels{noneLabel.c_str()};
    std::vector<int> interpolationIds{};
    std::vector<const char*> interpolationLabels{};
    for (int i = 0; i < (int)std::size(INTERPOLATION_LABELS); ++i)
    {
      interpolationIds.push_back(i);
      interpolationLabels.push_back(localize.get(INTERPOLATION_LABELS[i]));
    }
    auto regions =
        document.layer_regions_get(document.reference_get().itemType == LAYER ? document.reference_get().itemID : -1);
    if (!isLayerPropertyAvailable) regionId = shaderId = -1;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImGui::GetStyle().ItemInnerSpacing);
    rows_draw(componentRows, false);
    ImGui::PushID("Duration");
    property_toggle_begin(settings.changeIsDuration);
    input_int_range(localize.get(BASIC_DURATION), settings.changeDuration, FRAME_DURATION_MIN, FRAME_DURATION_MAX, STEP,
                    STEP_FAST);
    ImGui::EndDisabled();
    ImGui::PopID();
    rows_draw(colorRows, true);

    ImGui::BeginDisabled(!isLayerPropertyAvailable);
    combo_draw("Region", BASIC_REGION, settings.changeIsRegion, regionId, regions ? regions->ids : noneIds,
               regions ? regions->labels : noneLabels);
    ImGui::EndDisabled();
    combo_draw("Interpolation", BASIC_INTERPOLATED, settings.changeIsInterpolationSet, settings.changeInterpolation,
               interpolationIds, interpolationLabels);
    ImGui::BeginDisabled(!isLayerPropertyAvailable);
    combo_draw("Shader", BASIC_SHADER, settings.changeIsShader, shaderId,
               document.shader.ids.empty() ? noneIds : document.shader.ids,
               document.shader.labels.empty() ? noneLabels : document.shader.labels);
    ImGui::EndDisabled();
    bool_draw("Visible", BASIC_VISIBLE, settings.changeIsVisibleSet, settings.changeIsVisible);
    bool_draw("Flip X", LABEL_FLIP_X, settings.changeIsFlipXSet, settings.changeIsFlipX);
    ImGui::SameLine();
    bool_draw("Flip Y", LABEL_FLIP_Y, settings.changeIsFlipYSet, settings.changeIsFlipY);
    ImGui::PopStyleVar();

    auto frame_change = [&](ChangeType changeType)
    {
      auto isColorScaled = changeType == ChangeType::MULTIPLY || changeType == ChangeType::DIVIDE;
      FrameChange frameChange;
#define X(name, member, isLayerOnly, isColor, isEnabled, value)                                                        \
  if (settings.isEnabled)                                                                                              \
    frameChange.name = isColor && isColorScaled ? (float)float_to_uint8(settings.value) : settings.value;
      FRAME_CHANGE_SCALARS
#undef X
      if (settings.changeIsDuration) frameChange.duration = settings.changeDuration;
      if (settings.changeIsRegion) frameChange.regionId = regionId;
      if (settings.changeIsShader) frameChange.shaderId = shaderId;
      if (settings.changeIsVisibleSet) frameChange.isVisible = settings.changeIsVisible;
      if (settings.changeIsInterpolationSet) frameChange.interpolation = (Interpolation)settings.changeInterpolation;
      if (settings.changeIsFlipXSet) frameChange.isFlipX = settings.changeIsFlipX;
      if (settings.changeIsFlipYSet) frameChange.isFlipY = settings.changeIsFlipY;

      manager.command_push(
          {manager.selected,
           [=, queuedAnimations = std::set<int>(animations.begin(), animations.end()), isRoot = settings.changeIsRoot,
            isLayers = settings.changeIsLayers, isNulls = settings.changeIsNulls](Manager&, Document& document)
           {
             edit::ChangeTargets targets{.isRoot = isRoot, .isLayers = isLayers, .isNulls = isNulls};
             if (isFramesDestination)
               targets.references = {selectedFrameReferences.begin(), selectedFrameReferences.end()};
             else if (isItemsDestination)
               targets.references = {selectedItemReferences.begin(), selectedItemReferences.end()};
             else
               targets.animations = queuedAnimations;
             document.edit_apply(EDIT_CHANGE_FRAME_PROPERTIES, [&](Anm2& anm2)
                                 { return edit::frames_change_apply(anm2, targets, frameChange, changeType); });
           }});
      isChanged = true;
    };

    ImGui::Separator();

    if (isFromWizard)
    {
      ImGui::SeparatorText(localize.get(LABEL_DESTINATION));
      struct DestinationOption
      {
        StringType label;
        StringType tooltip;
        ChangeDestination value;
        bool isAvailable;
      };
      DestinationOption destinationOptions[] = {
          {BASIC_FRAMES, TOOLTIP_CHANGE_ALL_DESTINATION_FRAMES, CHANGE_DESTINATION_FRAMES, isSelectedFramesAvailable},
          {LABEL_ITEMS, TOOLTIP_CHANGE_ALL_DESTINATION_ITEMS, CHANGE_DESTINATION_ITEMS, isSelectedItemsAvailable},
          {LABEL_ANIMATIONS_CHILD, TOOLTIP_CHANGE_ALL_DESTINATION_ANIMATIONS, CHANGE_DESTINATION_ANIMATIONS,
           isSelectedAnimationsAvailable}};
      for (auto& option : destinationOptions)
      {
        if (option.value != CHANGE_DESTINATION_FRAMES) ImGui::SameLine();
        ImGui::BeginDisabled(!option.isAvailable);
        ImGui::RadioButton(localize.get(option.label), &destination, option.value);
        ImGui::SetItemTooltip("%s", localize.get(option.tooltip));
        ImGui::EndDisabled();
      }

      struct TargetOption
      {
        StringType label;
        StringType tooltip;
        bool& value;
      };
      TargetOption targetOptions[] = {{LABEL_ROOT, TOOLTIP_CHANGE_ALL_ROOT, settings.changeIsRoot},
                                      {LABEL_LAYERS, TOOLTIP_CHANGE_ALL_LAYERS, settings.changeIsLayers},
                                      {LABEL_NULLS, TOOLTIP_CHANGE_ALL_NULLS, settings.changeIsNulls}};
      ImGui::BeginDisabled(isFramesDestination || isItemsDestination);
      for (auto& option : targetOptions)
      {
        if (&option != targetOptions) ImGui::SameLine();
        ImGui::Checkbox(localize.get(option.label), &option.value);
        ImGui::SetItemTooltip("%s", localize.get(option.tooltip));
      }
      ImGui::EndDisabled();
      ImGui::Separator();
    }

    bool isAnyProperty = settings.changeIsDuration || settings.changeIsRegion || settings.changeIsShader ||
                         settings.changeIsVisibleSet || settings.changeIsInterpolationSet ||
                         settings.changeIsFlipXSet ||
                         settings.changeIsFlipYSet
#define X(name, member, isLayerOnly, isColor, isEnabled, value) || settings.isEnabled
                             FRAME_CHANGE_SCALARS
#undef X
        ;
    bool isDestinationValid = isFramesDestination ? isSelectedFramesAvailable
                              : isItemsDestination
                                  ? isSelectedItemsAvailable
                                  : isSelectedAnimationsAvailable &&
                                        (settings.changeIsRoot || settings.changeIsLayers || settings.changeIsNulls);

    auto rowWidgetSize = widget_size_with_row_get(CHANGE_TYPE_COUNT);
    ImGui::BeginDisabled(!isAnyProperty || !isDestinationValid);
    for (auto& button : CHANGE_BUTTONS)
    {
      if (&button != CHANGE_BUTTONS) ImGui::SameLine();
      if (ImGui::Button(localize.get(button.label), rowWidgetSize)) frame_change(button.type);
      ImGui::SetItemTooltip("%s", localize.get(button.tooltip));
    }
    ImGui::EndDisabled();
  }
}
