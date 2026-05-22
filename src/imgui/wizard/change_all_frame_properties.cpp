#include "change_all_frame_properties.hpp"

#include <algorithm>
#include <map>
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
  enum ChangeDestination
  {
    CHANGE_DESTINATION_FRAMES,
    CHANGE_DESTINATION_ANIMATIONS,
    CHANGE_DESTINATION_ITEMS,
  };

  void ChangeAllFrameProperties::update(Manager& manager, Document& document, Settings& settings, bool isFromWizard)
  {
    isChanged = false;

    auto& anm2 = document.anm2;
    auto& frames = document.frames.selection;
    auto& frameReferences = document.frames.references;
    auto& itemReferences = document.items.references;
    auto& animations = document.animation.selection;
    auto& isCropX = settings.changeIsCropX;
    auto& isCropY = settings.changeIsCropY;
    auto& isSizeX = settings.changeIsSizeX;
    auto& isSizeY = settings.changeIsSizeY;
    auto& isPositionX = settings.changeIsPositionX;
    auto& isPositionY = settings.changeIsPositionY;
    auto& isPivotX = settings.changeIsPivotX;
    auto& isPivotY = settings.changeIsPivotY;
    auto& isScaleX = settings.changeIsScaleX;
    auto& isScaleY = settings.changeIsScaleY;
    auto& isRotation = settings.changeIsRotation;
    auto& isDuration = settings.changeIsDuration;
    auto& isTintR = settings.changeIsTintR;
    auto& isTintG = settings.changeIsTintG;
    auto& isTintB = settings.changeIsTintB;
    auto& isTintA = settings.changeIsTintA;
    auto& isColorOffsetR = settings.changeIsColorOffsetR;
    auto& isColorOffsetG = settings.changeIsColorOffsetG;
    auto& isColorOffsetB = settings.changeIsColorOffsetB;
    auto& isVisibleSet = settings.changeIsVisibleSet;
    auto& isInterpolationSet = settings.changeIsInterpolationSet;
    auto& isFlipXSet = settings.changeIsFlipXSet;
    auto& isFlipYSet = settings.changeIsFlipYSet;
    auto& isRegion = settings.changeIsRegion;
    auto& crop = settings.changeCrop;
    auto& size = settings.changeSize;
    auto& position = settings.changePosition;
    auto& pivot = settings.changePivot;
    auto& scale = settings.changeScale;
    auto& rotation = settings.changeRotation;
    auto& duration = settings.changeDuration;
    auto& tint = settings.changeTint;
    auto& colorOffset = settings.changeColorOffset;
    auto& regionId = document.changeAllFramePropertiesRegionId;
    auto& isVisible = settings.changeIsVisible;
    auto& interpolation = settings.changeInterpolation;
    auto& isFlipX = settings.changeIsFlipX;
    auto& isFlipY = settings.changeIsFlipY;
    auto& destination = settings.changeDestination;
    auto& isRoot = settings.changeIsRoot;
    auto& isLayers = settings.changeIsLayers;
    auto& isNulls = settings.changeIsNulls;
    auto& itemType = document.reference.itemType;

    auto is_frame_reference_changeable = [](const Reference& reference) { return reference.itemType != TRIGGER; };
    auto is_item_reference_changeable = [](const Reference& reference) { return reference.itemType != TRIGGER; };
    auto selected_frame_references_get = [&]()
    {
      std::set<Reference> result = frameReferences;
      if (result.empty())
        for (auto frameIndex : frames)
          result.insert({document.reference.animationIndex, itemType, document.reference.itemID, frameIndex});
      std::erase_if(result, [&](const Reference& reference) { return !is_frame_reference_changeable(reference); });
      return result;
    };
    auto selected_item_references_get = [&]()
    {
      std::set<Reference> result = itemReferences;
      if (result.empty() && itemType != NONE)
        result.insert({document.reference.animationIndex, itemType, document.reference.itemID});
      std::erase_if(result, [&](const Reference& reference) { return !is_item_reference_changeable(reference); });
      return result;
    };
    auto selectedFrameReferences = selected_frame_references_get();
    auto selectedItemReferences = selected_item_references_get();

    bool isFramesDestination = !isFromWizard || destination == CHANGE_DESTINATION_FRAMES;
    bool isItemsDestination = isFromWizard && destination == CHANGE_DESTINATION_ITEMS;
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
      isFramesDestination = destination == CHANGE_DESTINATION_FRAMES;
      isItemsDestination = destination == CHANGE_DESTINATION_ITEMS;
    }

    auto isLayerPropertyAvailable = isFramesDestination
                                        ? std::ranges::any_of(selectedFrameReferences,
                                                              [](const Reference& reference)
                                                              { return reference.itemType == LAYER; })
                                    : isItemsDestination
                                        ? std::ranges::any_of(selectedItemReferences,
                                                              [](const Reference& reference)
                                                              { return reference.itemType == LAYER; })
                                        : isLayers;

#define PROPERTIES_WIDGET(body, checkboxLabel, isEnabled)                                                              \
  ImGui::Checkbox(checkboxLabel, &isEnabled);                                                                          \
  ImGui::SameLine();                                                                                                   \
  ImGui::BeginDisabled(!isEnabled);                                                                                    \
  body;                                                                                                                \
  ImGui::EndDisabled();

    auto bool_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, bool& value)
    { PROPERTIES_WIDGET(ImGui::Checkbox(valueLabel, &value), checkboxLabel, isEnabled) };

    auto enum_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, int& value,
                          const std::vector<int>& ids, std::vector<const char*>& labels)
    { PROPERTIES_WIDGET(combo_id_mapped(valueLabel, &value, ids, labels), checkboxLabel, isEnabled) };

    auto color3_value = [&](const char* checkboxRLabel, const char* checkboxGLabel, const char* checkboxBLabel,
                            const char* valueRLabel, const char* valueGLabel, const char* valueBLabel,
                            const char* label, bool& isREnabled, bool& isGEnabled, bool& isBEnabled, vec3& value)
    {
      auto style = ImGui::GetStyle();

      auto width = (ImGui::CalcItemWidth() - (ImGui::GetFrameHeightWithSpacing() * 2) - (style.ItemSpacing.x * 2) -
                    ImGui::GetFrameHeight()) /
                   3;

      ivec3 valueAlt = {float_to_uint8(value.r), float_to_uint8(value.g), float_to_uint8(value.b)};

      ImGui::PushItemWidth(width);

      PROPERTIES_WIDGET(ImGui::DragInt(valueRLabel, &valueAlt.r, DRAG_SPEED, 0, 255, "R:%d"), checkboxRLabel,
                        isREnabled);

      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueGLabel, &valueAlt.g, DRAG_SPEED, 0, 255, "G:%d"), checkboxGLabel,
                        isGEnabled);

      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueBLabel, &valueAlt.b, DRAG_SPEED, 0, 255, "B:%d"), checkboxBLabel,
                        isBEnabled);

      ImGui::PopItemWidth();

      ImGui::SameLine();

      value = vec3(uint8_to_float(valueAlt.r), uint8_to_float(valueAlt.g), uint8_to_float(valueAlt.b));

      ImVec4 buttonColor = {isREnabled ? value.r : 0, isGEnabled ? value.g : 0, isBEnabled ? value.b : 0, 1};

      ImGui::ColorButton(label, buttonColor);

      ImGui::SameLine();

      ImGui::TextUnformatted(label);
    };

    auto color4_value = [&](const char* checkboxRLabel, const char* checkboxGLabel, const char* checkboxBLabel,
                            const char* checkboxALabel, const char* valueRLabel, const char* valueGLabel,
                            const char* valueBLabel, const char* valueALabel, const char* label, bool& isREnabled,
                            bool& isGEnabled, bool& isBEnabled, bool& isAEnabled, vec4& value)
    {
      auto style = ImGui::GetStyle();
      auto width = (ImGui::CalcItemWidth() - (ImGui::GetFrameHeightWithSpacing() * 3) - (style.ItemSpacing.x * 3) -
                    ImGui::GetFrameHeight()) /
                   4;
      ivec4 valueAlt = {float_to_uint8(value.r), float_to_uint8(value.g), float_to_uint8(value.b),
                        float_to_uint8(value.a)};

      ImGui::PushItemWidth(width);

      PROPERTIES_WIDGET(ImGui::DragInt(valueRLabel, &valueAlt.r, DRAG_SPEED, 0, 255, "R:%d"), checkboxRLabel,
                        isREnabled);
      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueGLabel, &valueAlt.g, DRAG_SPEED, 0, 255, "G:%d"), checkboxGLabel,
                        isGEnabled);

      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueBLabel, &valueAlt.b, DRAG_SPEED, 0, 255, "B:%d"), checkboxBLabel,
                        isBEnabled);

      ImGui::SameLine();

      PROPERTIES_WIDGET(ImGui::DragInt(valueALabel, &valueAlt.a, DRAG_SPEED, 0, 255, "A:%d"), checkboxALabel,
                        isAEnabled);

      ImGui::PopItemWidth();

      ImGui::SameLine();

      value = vec4(uint8_to_float(valueAlt.r), uint8_to_float(valueAlt.g), uint8_to_float(valueAlt.b),
                   uint8_to_float(valueAlt.a));

      ImVec4 buttonColor = {isREnabled ? value.r : 0, isGEnabled ? value.g : 0, isBEnabled ? value.b : 0,
                            isAEnabled ? value.a : 1};
      ImGui::ColorButton(label, buttonColor);

      ImGui::SameLine();

      ImGui::TextUnformatted(label);
    };

    auto float_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, float& value)
    {
      PROPERTIES_WIDGET(ImGui::DragFloat(valueLabel, &value, DRAG_SPEED, 0.0f, 0.0f, float_format_get(value)),
                        checkboxLabel, isEnabled);
    };

    auto float2_value = [&](const char* checkboxXLabel, const char* checkboxYLabel, const char* valueXLabel,
                            const char* valueYLabel, bool& isXEnabled, bool& isYEnabled, vec2& value)
    {
      auto style = ImGui::GetStyle();

      auto width = (ImGui::CalcItemWidth() - ImGui::GetFrameHeightWithSpacing() - style.ItemSpacing.x) / 2;

      ImGui::PushItemWidth(width);
      PROPERTIES_WIDGET(ImGui::DragFloat(valueXLabel, &value.x, DRAG_SPEED, 0.0f, 0.0f, float_format_get(value.x)),
                        checkboxXLabel, isXEnabled);
      ImGui::SameLine();
      PROPERTIES_WIDGET(ImGui::DragFloat(valueYLabel, &value.y, DRAG_SPEED, 0.0f, 0.0f, float_format_get(value.y)),
                        checkboxYLabel, isYEnabled);
      ImGui::PopItemWidth();
    };

    auto duration_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, int& value)
    {
      PROPERTIES_WIDGET(input_int_range(valueLabel, value, FRAME_DURATION_MIN, FRAME_DURATION_MAX, STEP, STEP_FAST),
                        checkboxLabel, isEnabled);
    };

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImGui::GetStyle().ItemInnerSpacing);

    ImGui::BeginDisabled(!isLayerPropertyAvailable);
    float2_value("##Is Crop X", "##Is Crop Y", "##Crop X", localize.get(BASIC_CROP), isCropX, isCropY, crop);
    float2_value("##Is Size X", "##Is Size Y", "##Size X", localize.get(BASIC_SIZE), isSizeX, isSizeY, size);
    ImGui::EndDisabled();

    float2_value("##Is Position X", "##Is Position Y", "##Position X", localize.get(BASIC_POSITION), isPositionX,
                 isPositionY, position);

    ImGui::BeginDisabled(!isLayerPropertyAvailable);
    float2_value("##Is Pivot X", "##Is Pivot Y", "##Pivot X", localize.get(BASIC_PIVOT), isPivotX, isPivotY, pivot);
    ImGui::EndDisabled();

    float2_value("##Is Scale X", "##Is Scale Y", "##Scale X", localize.get(BASIC_SCALE), isScaleX, isScaleY, scale);

    float_value("##Is Rotation", localize.get(BASIC_ROTATION), isRotation, rotation);

    duration_value("##Is Duration", localize.get(BASIC_DURATION), isDuration, duration);

    color4_value("##Is Tint R", "##Is Tint G", "##Is Tint B", "##Is Tint A", "##Tint R", "##Tint G", "##Tint B",
                 "##Tint A", localize.get(BASIC_TINT), isTintR, isTintG, isTintB, isTintA, tint);

    color3_value("##Is Color Offset R", "##Is Color Offset G", "##Is Color Offset B", "##Color Offset R",
                 "##Color Offset B", "##Color Offset G", localize.get(BASIC_COLOR_OFFSET), isColorOffsetR,
                 isColorOffsetG, isColorOffsetB, colorOffset);

    ImGui::BeginDisabled(!isLayerPropertyAvailable);
    std::vector<int> fallbackIds{-1};
    std::vector<std::string> fallbackLabelsString{localize.get(BASIC_NONE)};
    std::vector<const char*> fallbackLabels{fallbackLabelsString[0].c_str()};
    std::vector<int> interpolationIds{(int)Interpolation::NONE, (int)Interpolation::LINEAR, (int)Interpolation::EASE_IN,
                                      (int)Interpolation::EASE_OUT, (int)Interpolation::EASE_IN_OUT};
    std::vector<std::string> interpolationLabelsString{localize.get(BASIC_NONE), localize.get(BASIC_LINEAR),
                                                       localize.get(BASIC_EASE_IN), localize.get(BASIC_EASE_OUT),
                                                       localize.get(BASIC_EASE_IN_OUT)};
    std::vector<const char*> interpolationLabels{
        interpolationLabelsString[0].c_str(), interpolationLabelsString[1].c_str(),
        interpolationLabelsString[2].c_str(), interpolationLabelsString[3].c_str(),
        interpolationLabelsString[4].c_str()};

    const Storage* regionStorage = nullptr;
    if (itemType == LAYER && document.reference.itemID != -1)
    {
      if (auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, document.reference.itemID))
      {
        auto regionIt = document.regionBySpritesheet.find(layer->spritesheetId);
        if (regionIt != document.regionBySpritesheet.end()) regionStorage = &regionIt->second;
      }
    }
    auto regionIds = regionStorage && !regionStorage->ids.empty() ? regionStorage->ids : fallbackIds;
    auto regionLabels = regionStorage && !regionStorage->labels.empty() ? regionStorage->labels : fallbackLabels;
    if (!isLayerPropertyAvailable || std::find(regionIds.begin(), regionIds.end(), regionId) == regionIds.end())
      regionId = -1;
    PROPERTIES_WIDGET(combo_id_mapped(localize.get(BASIC_REGION), &regionId, regionIds, regionLabels), "##Is Region",
                      isRegion);
    ImGui::EndDisabled();

    enum_value("##Is Interpolation", localize.get(BASIC_INTERPOLATED), isInterpolationSet, interpolation,
               interpolationIds, interpolationLabels);

    bool_value("##Is Visible", localize.get(BASIC_VISIBLE), isVisibleSet, isVisible);

    bool_value("##Is Flip X", localize.get(LABEL_FLIP_X), isFlipXSet, isFlipX);

    ImGui::SameLine();

    bool_value("##Is Flip Y", localize.get(LABEL_FLIP_Y), isFlipYSet, isFlipY);

    ImGui::PopStyleVar();

#undef PROPERTIES_WIDGET

    auto frame_change = [&](ChangeType changeType)
    {
      FrameChange frameChange;
      if (isCropX) frameChange.cropX = crop.x;
      if (isCropY) frameChange.cropY = crop.y;
      if (isSizeX) frameChange.sizeX = size.x;
      if (isSizeY) frameChange.sizeY = size.y;
      if (isPositionX) frameChange.positionX = position.x;
      if (isPositionY) frameChange.positionY = position.y;
      if (isPivotX) frameChange.pivotX = pivot.x;
      if (isPivotY) frameChange.pivotY = pivot.y;
      if (isScaleX) frameChange.scaleX = scale.x;
      if (isScaleY) frameChange.scaleY = scale.y;
      if (isRotation) frameChange.rotation = std::make_optional(rotation);
      if (isDuration) frameChange.duration = std::make_optional(duration);
      if (isRegion) frameChange.regionId = std::make_optional(regionId);
      if (isTintR) frameChange.tintR = tint.r;
      if (isTintG) frameChange.tintG = tint.g;
      if (isTintB) frameChange.tintB = tint.b;
      if (isTintA) frameChange.tintA = tint.a;
      if (isColorOffsetR) frameChange.colorOffsetR = colorOffset.r;
      if (isColorOffsetG) frameChange.colorOffsetG = colorOffset.g;
      if (isColorOffsetB) frameChange.colorOffsetB = colorOffset.b;
      if (isVisibleSet) frameChange.isVisible = std::make_optional(isVisible);
      if (isInterpolationSet) frameChange.interpolation = std::make_optional(static_cast<Interpolation>(interpolation));
      if (isFlipXSet) frameChange.isFlipX = std::make_optional(isFlipX);
      if (isFlipYSet) frameChange.isFlipY = std::make_optional(isFlipY);

      if (isFramesDestination && selectedFrameReferences.empty()) return;
      if (isItemsDestination && selectedItemReferences.empty()) return;

      auto queuedFrameReferences = selectedFrameReferences;
      auto queuedItemReferences = selectedItemReferences;
      auto queuedAnimations = animations;
      auto queuedIsFramesDestination = isFramesDestination;
      auto queuedIsItemsDestination = isItemsDestination;
      auto queuedIsRoot = isRoot;
      auto queuedIsLayers = isLayers;
      auto queuedIsNulls = isNulls;

      manager.command_push({manager.selected,
                            [=](Manager&, Document& document)
                            {
                              auto all_frames_selection = [](const Element& item)
                              {
                                std::set<int> selection{};
                                int index{};
                                for (const auto& frame : item.children)
                                  if (frame.type == ElementType::FRAME)
                                  {
                                    selection.insert(index);
                                    ++index;
                                  }
                                return selection;
                              };

                              auto& anm2 = document.anm2;

                              if (queuedIsFramesDestination)
                              {
                                std::map<Reference, std::set<int>> groupedFrames{};
                                for (auto frameReference : queuedFrameReferences)
                                {
                                  auto itemReference = frameReference;
                                  itemReference.frameIndex = -1;
                                  groupedFrames[itemReference].insert(frameReference.frameIndex);
                                }

                                document.snapshot(localize.get(EDIT_CHANGE_FRAME_PROPERTIES));
                                for (auto& [itemReference, selection] : groupedFrames)
                                {
                                  auto item = anm2.element_get(itemReference.animationIndex,
                                                               static_cast<ItemType>(itemReference.itemType),
                                                               itemReference.itemID);
                                  if (!item) continue;
                                  frames_change(*item, frameChange, static_cast<ItemType>(itemReference.itemType),
                                                changeType, selection);
                                }
                                document.anm2_change(Document::FRAMES);
                                return;
                              }

                              if (queuedIsItemsDestination)
                              {
                                document.snapshot(localize.get(EDIT_CHANGE_FRAME_PROPERTIES));
                                for (auto itemReference : queuedItemReferences)
                                {
                                  if (itemReference.itemType == TRIGGER) continue;
                                  auto item = anm2.element_get(itemReference.animationIndex,
                                                               static_cast<ItemType>(itemReference.itemType),
                                                               itemReference.itemID);
                                  if (!item) continue;
                                  auto selection = all_frames_selection(*item);
                                  frames_change(*item, frameChange, static_cast<ItemType>(itemReference.itemType),
                                                changeType, selection);
                                }
                                document.anm2_change(Document::FRAMES);
                                return;
                              }

                              document.snapshot(localize.get(EDIT_CHANGE_FRAME_PROPERTIES));
                              for (auto animationIndex : queuedAnimations)
                              {
                                auto animation = anm2.element_get(ElementType::ANIMATION, animationIndex);
                                if (!animation) continue;

                                if (queuedIsRoot)
                                {
                                  if (auto item = animation_item_get(*animation, ItemType::ROOT))
                                  {
                                    auto selection = all_frames_selection(*item);
                                    frames_change(*item, frameChange, ItemType::ROOT, changeType, selection);
                                  }
                                }

                                if (queuedIsLayers)
                                {
                                  auto layerAnimations = element_child_first_get(*animation, ElementType::LAYER_ANIMATIONS);
                                  if (layerAnimations)
                                  {
                                    auto item_change = [&](auto&& self, Element& item) -> void
                                    {
                                      if (item.type == ElementType::GROUP)
                                      {
                                        for (auto& child : item.children)
                                          self(self, child);
                                        return;
                                      }
                                      if (item.type == ElementType::LAYER_ANIMATION)
                                      {
                                        auto selection = all_frames_selection(item);
                                        frames_change(item, frameChange, ItemType::LAYER, changeType, selection);
                                      }
                                    };
                                    for (auto& item : layerAnimations->children)
                                      item_change(item_change, item);
                                  }
                                }

                                if (queuedIsNulls)
                                {
                                  auto nullAnimations = element_child_first_get(*animation, ElementType::NULL_ANIMATIONS);
                                  if (nullAnimations)
                                  {
                                    auto item_change = [&](auto&& self, Element& item) -> void
                                    {
                                      if (item.type == ElementType::GROUP)
                                      {
                                        for (auto& child : item.children)
                                          self(self, child);
                                        return;
                                      }
                                      if (item.type == ElementType::NULL_ANIMATION)
                                      {
                                        auto selection = all_frames_selection(item);
                                        frames_change(item, frameChange, ItemType::NULL_, changeType, selection);
                                      }
                                    };
                                    for (auto& item : nullAnimations->children)
                                      item_change(item_change, item);
                                  }
                                }
                              }
                              document.anm2_change(Document::FRAMES);
                            }});
      isChanged = true;
    };

    ImGui::Separator();

    if (isFromWizard)
    {
      ImGui::SeparatorText(localize.get(LABEL_DESTINATION));

      ImGui::BeginDisabled(!isSelectedFramesAvailable);
      ImGui::RadioButton(localize.get(BASIC_FRAMES), &destination, CHANGE_DESTINATION_FRAMES);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CHANGE_ALL_DESTINATION_FRAMES));
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(!isSelectedItemsAvailable);
      ImGui::RadioButton(localize.get(LABEL_ITEMS), &destination, CHANGE_DESTINATION_ITEMS);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CHANGE_ALL_DESTINATION_ITEMS));
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(!isSelectedAnimationsAvailable);
      ImGui::RadioButton(localize.get(LABEL_ANIMATIONS_CHILD), &destination, CHANGE_DESTINATION_ANIMATIONS);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CHANGE_ALL_DESTINATION_ANIMATIONS));
      ImGui::EndDisabled();

      ImGui::BeginDisabled(isFramesDestination || isItemsDestination);
      ImGui::Checkbox(localize.get(LABEL_ROOT), &isRoot);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CHANGE_ALL_ROOT));
      ImGui::SameLine();
      ImGui::Checkbox(localize.get(LABEL_LAYERS), &isLayers);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CHANGE_ALL_LAYERS));
      ImGui::SameLine();
      ImGui::Checkbox(localize.get(LABEL_NULLS), &isNulls);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CHANGE_ALL_NULLS));
      ImGui::EndDisabled();

      ImGui::Separator();
    }

    bool isAnyProperty = isCropX || isCropY || isSizeX || isSizeY || isPositionX || isPositionY || isPivotX ||
                         isPivotY || isScaleX || isScaleY || isRotation || isDuration || isTintR || isTintG ||
                         isTintB || isTintA || isColorOffsetR || isColorOffsetG || isColorOffsetB || isRegion ||
                         isVisibleSet || isInterpolationSet || isFlipXSet || isFlipYSet;
    bool isDestinationValid = isFramesDestination ? isSelectedFramesAvailable
                              : isItemsDestination ? isSelectedItemsAvailable
                                                   : isSelectedAnimationsAvailable && (isRoot || isLayers || isNulls);

    auto rowWidgetSize = widget_size_with_row_get(5);

    ImGui::BeginDisabled(!isAnyProperty || !isDestinationValid);

    if (ImGui::Button(localize.get(LABEL_ADJUST), rowWidgetSize)) frame_change(ChangeType::ADJUST);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADJUST));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(BASIC_ADD), rowWidgetSize)) frame_change(ChangeType::ADD);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADD_VALUES));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_SUBTRACT), rowWidgetSize)) frame_change(ChangeType::SUBTRACT);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SUBTRACT_VALUES));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_MULTIPLY), rowWidgetSize)) frame_change(ChangeType::MULTIPLY);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MULTIPLY_VALUES));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_DIVIDE), rowWidgetSize)) frame_change(ChangeType::DIVIDE);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_DIVIDE_VALUES));

    ImGui::EndDisabled();
  }
}
