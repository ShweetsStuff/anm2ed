#pragma once

#include <cmath>
#include <cstdint>
#include <optional>

#include <glm/glm.hpp>

#include "types.hpp"

#define ANM2_ELEMENT_TYPES                                                                                             \
  X(UNKNOWN, "")                                                                                                       \
  X(ANIMATED_ACTOR, "AnimatedActor")                                                                                   \
  X(INFO, "Info")                                                                                                      \
  X(CONTENT, "Content")                                                                                                \
  X(SPRITESHEETS, "Spritesheets")                                                                                      \
  X(SPRITESHEET, "Spritesheet")                                                                                        \
  X(SHADERS, "Shaders")                                                                                                \
  X(SHADER, "Shader")                                                                                                  \
  X(UNIFORM, "Uniform")                                                                                                \
  X(COMPONENT, "Component")                                                                                            \
  X(REGION, "Region")                                                                                                  \
  X(LAYERS, "Layers")                                                                                                  \
  X(LAYER_ELEMENT, "Layer")                                                                                            \
  X(NULLS, "Nulls")                                                                                                    \
  X(NULL_ELEMENT, "Null")                                                                                              \
  X(EVENTS, "Events")                                                                                                  \
  X(EVENT_ELEMENT, "Event")                                                                                            \
  X(SOUNDS, "Sounds")                                                                                                  \
  X(SOUND_ELEMENT, "Sound")                                                                                            \
  X(ANIMATIONS, "Animations")                                                                                          \
  X(ANIMATION, "Animation")                                                                                            \
  X(ROOT_ANIMATION, "RootAnimation")                                                                                   \
  X(LAYER_ANIMATIONS, "LayerAnimations")                                                                               \
  X(LAYER_ANIMATION_GROUPS, "LayerAnimationGroups")                                                                    \
  X(LAYER_ANIMATION, "LayerAnimation")                                                                                 \
  X(NULL_ANIMATIONS, "NullAnimations")                                                                                 \
  X(NULL_ANIMATION_GROUPS, "NullAnimationGroups")                                                                      \
  X(NULL_ANIMATION, "NullAnimation")                                                                                   \
  X(GROUP, "Group")                                                                                                    \
  X(TRIGGERS, "Triggers")                                                                                              \
  X(FRAME, "Frame")                                                                                                    \
  X(TRIGGER, "Trigger")

namespace anm2ed
{
  enum class ElementType
  {
#define X(symbol, tag) symbol,
    ANM2_ELEMENT_TYPES
#undef X
        COUNT
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
