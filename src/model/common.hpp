#pragma once

#include <cmath>
#include <compare>
#include <cstdint>
#include <optional>

#include <glm/glm.hpp>

#include "icon.hpp"
#include "strings.hpp"
#include "types.hpp"

#define ANM2_ELEMENT_TYPES                                                                                             \
  X(UNKNOWN, "", UNKNOWN)                                                                                              \
  X(ANIMATED_ACTOR, "AnimatedActor", UNKNOWN)                                                                          \
  X(INFO, "Info", UNKNOWN)                                                                                             \
  X(CONTENT, "Content", UNKNOWN)                                                                                       \
  X(SPRITESHEETS, "Spritesheets", UNKNOWN)                                                                             \
  X(SPRITESHEET, "Spritesheet", SPRITESHEETS)                                                                          \
  X(SHADERS, "Shaders", UNKNOWN)                                                                                       \
  X(SHADER, "Shader", SHADERS)                                                                                         \
  X(UNIFORM, "Uniform", UNKNOWN)                                                                                       \
  X(COMPONENT, "Component", UNKNOWN)                                                                                   \
  X(REGION, "Region", UNKNOWN)                                                                                         \
  X(LAYERS, "Layers", UNKNOWN)                                                                                         \
  X(LAYER_ELEMENT, "Layer", LAYERS)                                                                                    \
  X(NULLS, "Nulls", UNKNOWN)                                                                                           \
  X(NULL_ELEMENT, "Null", NULLS)                                                                                       \
  X(EVENTS, "Events", UNKNOWN)                                                                                         \
  X(EVENT_ELEMENT, "Event", EVENTS)                                                                                    \
  X(SOUNDS, "Sounds", UNKNOWN)                                                                                         \
  X(SOUND_ELEMENT, "Sound", SOUNDS)                                                                                    \
  X(ANIMATIONS, "Animations", UNKNOWN)                                                                                 \
  X(ANIMATION, "Animation", ANIMATIONS)                                                                                \
  X(ROOT_ANIMATION, "RootAnimation", UNKNOWN)                                                                          \
  X(LAYER_ANIMATIONS, "LayerAnimations", UNKNOWN)                                                                      \
  X(LAYER_ANIMATION_GROUPS, "LayerAnimationGroups", UNKNOWN)                                                           \
  X(LAYER_ANIMATION, "LayerAnimation", UNKNOWN)                                                                        \
  X(NULL_ANIMATIONS, "NullAnimations", UNKNOWN)                                                                        \
  X(NULL_ANIMATION_GROUPS, "NullAnimationGroups", UNKNOWN)                                                             \
  X(NULL_ANIMATION, "NullAnimation", UNKNOWN)                                                                          \
  X(GROUP, "Group", UNKNOWN)                                                                                           \
  X(TRIGGERS, "Triggers", UNKNOWN)                                                                                     \
  X(FRAME, "Frame", UNKNOWN)                                                                                           \
  X(TRIGGER, "Trigger", UNKNOWN)

namespace anm2ed
{
  enum class ElementType
  {
#define X(symbol, tag, container) symbol,
    ANM2_ELEMENT_TYPES
#undef X
        COUNT
  };

  inline constexpr ElementType ELEMENT_CONTAINERS[] = {
#define X(symbol, tag, container) ElementType::container,
      ANM2_ELEMENT_TYPES
#undef X
  };

  enum class Interpolation
  {
    NONE,
    LINEAR,
    EASE_IN,
    EASE_OUT,
    EASE_IN_OUT,
    COUNT
  };

  enum class Origin
  {
    CUSTOM,
    TOP_LEFT,
    CENTER,
    COUNT
  };

  enum class ItemType
  {
    NONE,
    ROOT,
    LAYER,
    NULL_,
    TRIGGER,
    COUNT
  };

  inline constexpr int NONE = (int)ItemType::NONE;
  inline constexpr int ROOT = (int)ItemType::ROOT;
  inline constexpr int LAYER = (int)ItemType::LAYER;
  inline constexpr int NULL_ = (int)ItemType::NULL_;
  inline constexpr int TRIGGER = (int)ItemType::TRIGGER;

  inline constexpr int FRAME_DURATION_MIN = 1;
  inline constexpr int FRAME_DURATION_MAX = 1000000;
  inline constexpr int FRAME_NUM_MIN = 1;
  inline constexpr int FRAME_NUM_MAX = FRAME_DURATION_MAX;
  inline constexpr int FPS_MIN = 1;
  inline constexpr int FPS_MAX = 120;

  enum class SpritesheetMergeOrigin
  {
    APPEND_RIGHT,
    APPEND_BOTTOM,
    COUNT
  };

  inline constexpr int APPEND_RIGHT = (int)SpritesheetMergeOrigin::APPEND_RIGHT;
  inline constexpr int APPEND_BOTTOM = (int)SpritesheetMergeOrigin::APPEND_BOTTOM;

  inline const glm::vec4 ROOT_COLOR = glm::vec4(0.140f, 0.310f, 0.560f, 1.000f);
  inline const glm::vec4 ROOT_COLOR_ACTIVE = glm::vec4(0.240f, 0.520f, 0.880f, 1.000f);
  inline const glm::vec4 ROOT_COLOR_HOVERED = glm::vec4(0.320f, 0.640f, 1.000f, 1.000f);

  inline const glm::vec4 LAYER_COLOR = glm::vec4(0.640f, 0.320f, 0.110f, 1.000f);
  inline const glm::vec4 LAYER_COLOR_ACTIVE = glm::vec4(0.840f, 0.450f, 0.170f, 1.000f);
  inline const glm::vec4 LAYER_COLOR_HOVERED = glm::vec4(0.960f, 0.560f, 0.240f, 1.000f);

  inline const glm::vec4 NULL_COLOR = glm::vec4(0.140f, 0.430f, 0.200f, 1.000f);
  inline const glm::vec4 NULL_COLOR_ACTIVE = glm::vec4(0.250f, 0.650f, 0.350f, 1.000f);
  inline const glm::vec4 NULL_COLOR_HOVERED = glm::vec4(0.350f, 0.800f, 0.480f, 1.000f);

  inline const glm::vec4 TRIGGER_COLOR = glm::vec4(0.620f, 0.150f, 0.260f, 1.000f);
  inline const glm::vec4 TRIGGER_COLOR_ACTIVE = glm::vec4(0.820f, 0.250f, 0.380f, 1.000f);
  inline const glm::vec4 TRIGGER_COLOR_HOVERED = glm::vec4(0.950f, 0.330f, 0.490f, 1.000f);

#define ANM2_ITEM_TYPES                                                                                                \
  X(NONE, STRING_UNDEFINED, "", resource::icon::NONE, glm::vec4(), glm::vec4(), glm::vec4(), UNKNOWN, UNKNOWN)         \
  X(ROOT, BASIC_ROOT, "RootAnimation", resource::icon::ROOT, ROOT_COLOR, ROOT_COLOR_ACTIVE, ROOT_COLOR_HOVERED,        \
    ROOT_ANIMATION, UNKNOWN)                                                                                           \
  X(LAYER, BASIC_LAYER_ANIMATION, "LayerAnimation", resource::icon::LAYER, LAYER_COLOR, LAYER_COLOR_ACTIVE,            \
    LAYER_COLOR_HOVERED, LAYER_ANIMATION, LAYER_ANIMATIONS)                                                            \
  X(NULL_, BASIC_NULL_ANIMATION, "NullAnimation", resource::icon::NULL_, NULL_COLOR, NULL_COLOR_ACTIVE,                \
    NULL_COLOR_HOVERED, NULL_ANIMATION, NULL_ANIMATIONS)                                                               \
  X(TRIGGER, BASIC_TRIGGERS, "Triggers", resource::icon::TRIGGERS, TRIGGER_COLOR, TRIGGER_COLOR_ACTIVE,                \
    TRIGGER_COLOR_HOVERED, TRIGGERS, UNKNOWN)

  constexpr StringType TYPE_STRINGS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered, track, container) string,
      ANM2_ITEM_TYPES
#undef X
  };

  constexpr const char* TYPE_ITEM_STRINGS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered, track, container) itemString,
      ANM2_ITEM_TYPES
#undef X
  };

  constexpr resource::icon::Type TYPE_ICONS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered, track, container) icon,
      ANM2_ITEM_TYPES
#undef X
  };

  inline const glm::vec4 TYPE_COLOR[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered, track, container) color,
      ANM2_ITEM_TYPES
#undef X
  };

  inline const glm::vec4 TYPE_COLOR_ACTIVE[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered, track, container) colorActive,
      ANM2_ITEM_TYPES
#undef X
  };

  inline const glm::vec4 TYPE_COLOR_HOVERED[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered, track, container) colorHovered,
      ANM2_ITEM_TYPES
#undef X
  };

  inline constexpr ElementType TYPE_TRACKS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered, track, container) ElementType::track,
      ANM2_ITEM_TYPES
#undef X
  };

  inline constexpr ElementType TYPE_CONTAINERS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered, track, container) ElementType::container,
      ANM2_ITEM_TYPES
#undef X
  };

  enum class RegionFrameMapping
  {
    PRESERVE,
    SET
  };

  enum FileMergePreset
  {
    FILE_MERGE_PRESET_MERGE_BY_NAME,
    FILE_MERGE_PRESET_APPEND_AS_NEW,
    FILE_MERGE_PRESET_REPLACE_MATCHING,
    FILE_MERGE_PRESET_COUNT
  };

  enum class ChangeType
  {
    ADJUST,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
  };

  enum Flag
  {
    SERIALIZE_GROUPS = 1 << 0,
    SERIALIZE_REGIONS = 1 << 1,
    SERIALIZE_SOUNDS = 1 << 2,
    SERIALIZE_REDUNDANT_FRAME_REGION_VALUES = 1 << 3,
    SERIALIZE_BAKE_SPECIAL_INTERPOLATED_FRAMES = 1 << 4,
    SERIALIZE_BAKE_GROUP_FRAMES = 1 << 5,
    SERIALIZE_NESTED_GROUPS = 1 << 6,
    SERIALIZE_EXTENSIONS = 1 << 7,
    SERIALIZE_FLATTEN_SPECIAL_INTERPOLATED_FRAMES = 1 << 8
  };

  using Flags = int;

  constexpr bool has_flag(Flags flags, Flag flag) { return (flags & flag) != 0; }
  constexpr Flags SERIALIZE_EDITOR_DEFAULT = SERIALIZE_GROUPS | SERIALIZE_REGIONS | SERIALIZE_SOUNDS |
                                             SERIALIZE_REDUNDANT_FRAME_REGION_VALUES | SERIALIZE_EXTENSIONS;
  constexpr Flags SERIALIZE_DEFAULT =
      SERIALIZE_EDITOR_DEFAULT | SERIALIZE_BAKE_SPECIAL_INTERPOLATED_FRAMES | SERIALIZE_BAKE_GROUP_FRAMES;
  constexpr Flags SERIALIZE_ISAAC_DEFAULT = SERIALIZE_BAKE_GROUP_FRAMES | SERIALIZE_FLATTEN_SPECIAL_INTERPOLATED_FRAMES;
  constexpr Flags SERIALIZE_ANM2ED_DEFAULT =
      SERIALIZE_GROUPS | SERIALIZE_REGIONS | SERIALIZE_SOUNDS | SERIALIZE_NESTED_GROUPS | SERIALIZE_EXTENSIONS;

  struct Options
  {
    Flags flags{SERIALIZE_DEFAULT};
    bool isExtendedFormat{};
  };

#define FRAME_CHANGE_SCALARS                                                                                           \
  X(cropX, crop.x, true, false, changeIsCropX, changeCrop.x)                                                           \
  X(cropY, crop.y, true, false, changeIsCropY, changeCrop.y)                                                           \
  X(sizeX, size.x, true, false, changeIsSizeX, changeSize.x)                                                           \
  X(sizeY, size.y, true, false, changeIsSizeY, changeSize.y)                                                           \
  X(pivotX, pivot.x, true, false, changeIsPivotX, changePivot.x)                                                       \
  X(pivotY, pivot.y, true, false, changeIsPivotY, changePivot.y)                                                       \
  X(positionX, position.x, false, false, changeIsPositionX, changePosition.x)                                          \
  X(positionY, position.y, false, false, changeIsPositionY, changePosition.y)                                          \
  X(scaleX, scale.x, false, false, changeIsScaleX, changeScale.x)                                                      \
  X(scaleY, scale.y, false, false, changeIsScaleY, changeScale.y)                                                      \
  X(shearX, shear.x, false, false, changeIsShearX, changeShear.x)                                                      \
  X(shearY, shear.y, false, false, changeIsShearY, changeShear.y)                                                      \
  X(rotation, rotation, false, false, changeIsRotation, changeRotation)                                                \
  X(tintR, tint.r, false, true, changeIsTintR, changeTint.r)                                                           \
  X(tintG, tint.g, false, true, changeIsTintG, changeTint.g)                                                           \
  X(tintB, tint.b, false, true, changeIsTintB, changeTint.b)                                                           \
  X(tintA, tint.a, false, true, changeIsTintA, changeTint.a)                                                           \
  X(colorOffsetR, colorOffset.r, false, true, changeIsColorOffsetR, changeColorOffset.r)                               \
  X(colorOffsetG, colorOffset.g, false, true, changeIsColorOffsetG, changeColorOffset.g)                               \
  X(colorOffsetB, colorOffset.b, false, true, changeIsColorOffsetB, changeColorOffset.b)

  struct FrameChange
  {
    std::optional<int> regionId{};
#define X(name, member, isLayerOnly, isColor, isEnabled, value) std::optional<float> name{};
    FRAME_CHANGE_SCALARS
#undef X
    std::optional<int> duration{};
    std::optional<int> shaderId{};
    std::optional<bool> isVisible{};
    std::optional<Interpolation> interpolation{};
    std::optional<bool> isFlipX{};
    std::optional<bool> isFlipY{};
  };

  inline constexpr StringType INTERPOLATION_LABELS[] = {BASIC_NONE, BASIC_LINEAR, BASIC_EASE_IN, BASIC_EASE_OUT,
                                                        BASIC_EASE_IN_OUT};

  struct Reference
  {
    int animationIndex{-1};
    int itemType{(int)ItemType::NONE};
    int itemID{-1};
    int frameIndex{-1};
    int groupType{(int)ItemType::NONE};
    int groupId{-1};

    auto operator<=>(const Reference&) const = default;
  };

  // A Reference expressed by runtime uids, so it survives inserts, deletes and moves.
  struct Handle
  {
    std::uint64_t animation{};
    std::uint64_t item{};
    std::uint64_t frame{};

    auto operator<=>(const Handle&) const = default;
  };

  inline float interpolation_factor(Interpolation interpolation, float value)
  {
    value = glm::clamp(value, 0.0f, 1.0f);
    if (interpolation == Interpolation::LINEAR) return value;
    if (interpolation == Interpolation::EASE_IN) return value * value;
    if (interpolation == Interpolation::EASE_OUT) return 1.0f - ((1.0f - value) * (1.0f - value));
    if (interpolation == Interpolation::EASE_IN_OUT)
      return value < 0.5f ? (2.0f * value * value) : (1.0f - std::pow(-2.0f * value + 2.0f, 2.0f) * 0.5f);
    return 0.0f;
  }
}
