#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>
#include <tinyxml2/tinyxml2.h>

#include "icon.hpp"
#include "strings.hpp"
#include "types.hpp"

#define ANM2_ELEMENT_TYPES                                                                                             \
  X(UNKNOWN, "")                                                                                                       \
  X(ANIMATED_ACTOR, "AnimatedActor")                                                                                   \
  X(INFO, "Info")                                                                                                      \
  X(CONTENT, "Content")                                                                                                \
  X(SPRITESHEETS, "Spritesheets")                                                                                      \
  X(SPRITESHEET, "Spritesheet")                                                                                        \
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
  X(LAYER_ANIMATION, "LayerAnimation")                                                                                 \
  X(NULL_ANIMATIONS, "NullAnimations")                                                                                 \
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

  enum class Compatibility
  {
    ISAAC,
    ANM2ED,
    ANM2ED_LIMITED,
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

  inline constexpr int ISAAC = (int)Compatibility::ISAAC;
  inline constexpr int ANM2ED = (int)Compatibility::ANM2ED;
  inline constexpr int ANM2ED_LIMITED = (int)Compatibility::ANM2ED_LIMITED;

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
  X(NONE, STRING_UNDEFINED, "", resource::icon::NONE, glm::vec4(), glm::vec4(), glm::vec4())                           \
  X(ROOT, BASIC_ROOT, "RootAnimation", resource::icon::ROOT, ROOT_COLOR, ROOT_COLOR_ACTIVE, ROOT_COLOR_HOVERED)        \
  X(LAYER, BASIC_LAYER_ANIMATION, "LayerAnimation", resource::icon::LAYER, LAYER_COLOR, LAYER_COLOR_ACTIVE,            \
    LAYER_COLOR_HOVERED)                                                                                               \
  X(NULL_, BASIC_NULL_ANIMATION, "NullAnimation", resource::icon::NULL_, NULL_COLOR, NULL_COLOR_ACTIVE,                \
    NULL_COLOR_HOVERED)                                                                                                \
  X(TRIGGER, BASIC_TRIGGERS, "Triggers", resource::icon::TRIGGERS, TRIGGER_COLOR, TRIGGER_COLOR_ACTIVE,                \
    TRIGGER_COLOR_HOVERED)

  constexpr StringType TYPE_STRINGS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) string,
      ANM2_ITEM_TYPES
#undef X
  };

  constexpr const char* TYPE_ITEM_STRINGS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) itemString,
      ANM2_ITEM_TYPES
#undef X
  };

  constexpr resource::icon::Type TYPE_ICONS[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) icon,
      ANM2_ITEM_TYPES
#undef X
  };

  inline const glm::vec4 TYPE_COLOR[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) color,
      ANM2_ITEM_TYPES
#undef X
  };

  inline const glm::vec4 TYPE_COLOR_ACTIVE[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) colorActive,
      ANM2_ITEM_TYPES
#undef X
  };

  inline const glm::vec4 TYPE_COLOR_HOVERED[] = {
#define X(symbol, string, itemString, icon, color, colorActive, colorHovered) colorHovered,
      ANM2_ITEM_TYPES
#undef X
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
    NO_SOUNDS = 1 << 0,
    NO_REGIONS = 1 << 1,
    FRAME_NO_REGION_VALUES = 1 << 2,
    INTERPOLATION_BOOL_ONLY = 1 << 3,
    NO_GROUPS = 1 << 4
  };

  using Flags = int;

  constexpr bool has_flag(Flags flags, Flag flag) { return (flags & flag) != 0; }

  struct Options
  {
    Compatibility compatibility{Compatibility::ANM2ED};
    bool isBakeSpecialInterpolatedFrames{};
    bool isRoundScale{true};
    bool isRoundRotation{true};
  };

  struct FrameChange
  {
    std::optional<bool> isVisible{};
    std::optional<Interpolation> interpolation{};
    std::optional<float> rotation{};
    std::optional<int> duration{};
    std::optional<int> regionId{};
    std::optional<float> pivotX{};
    std::optional<float> pivotY{};
    std::optional<float> cropX{};
    std::optional<float> cropY{};
    std::optional<float> positionX{};
    std::optional<float> positionY{};
    std::optional<float> sizeX{};
    std::optional<float> sizeY{};
    std::optional<float> scaleX{};
    std::optional<float> scaleY{};
    std::optional<float> colorOffsetR{};
    std::optional<float> colorOffsetG{};
    std::optional<float> colorOffsetB{};
    std::optional<float> tintR{};
    std::optional<float> tintG{};
    std::optional<float> tintB{};
    std::optional<float> tintA{};
    std::optional<bool> isFlipX{};
    std::optional<bool> isFlipY{};
  };

  struct Element
  {
    ElementType type{ElementType::UNKNOWN};
    std::string tag{};
    std::vector<Element> children{};
    std::string name{};
    std::string createdBy{"robot"};
    std::string createdOn{};
    std::filesystem::path path{};
    std::string defaultAnimation{};
    int id{-1};
    int layerId{-1};
    int nullId{-1};
    int spritesheetId{};
    int fps{30};
    int version{};
    int frameNum{1};
    int duration{1};
    int atFrame{-1};
    int eventId{-1};
    int regionId{-1};
    int soundId{-1};
    int groupId{-1};
    bool isLoop{true};
    bool isVisible{true};
    bool isShowRect{};
    bool isExpanded{true};
    Interpolation interpolation{Interpolation::NONE};
    Origin origin{Origin::CUSTOM};
    float rotation{};
    std::vector<int> soundIds{};
    glm::vec2 pivot{};
    glm::vec2 crop{};
    glm::vec2 position{};
    glm::vec2 size{};
    glm::vec2 scale{100.0f, 100.0f};
    glm::vec3 colorOffset{};
    glm::vec4 tint{types::color::WHITE};
  };

  struct Reference
  {
    int animationIndex{-1};
    int itemType{(int)ItemType::NONE};
    int itemID{-1};
    int frameIndex{-1};

    auto operator<=>(const Reference&) const = default;
  };

  Flags flags_for(Compatibility);
  Element element_make(ElementType);
  Element element_read(const tinyxml2::XMLElement*);
  std::string element_to_string(const Element&, Flags = 0);
  std::string element_to_string(const Element&, ElementType, Flags = 0);
  tinyxml2::XMLElement* element_to_xml(tinyxml2::XMLDocument&, const Element&, Flags = 0);
  tinyxml2::XMLElement* element_to_xml(tinyxml2::XMLDocument&, const Element&, ElementType, Flags = 0);
  Element* element_child_first_get(Element&, ElementType);
  const Element* element_child_first_get(const Element&, ElementType);
  Element* element_child_id_get(Element&, ElementType, int);
  const Element* element_child_id_get(const Element&, ElementType, int);
  int element_child_next_id_get(const Element&, ElementType);
  int element_child_max_id_get(const Element&, ElementType);
  bool element_child_id_erase(Element&, ElementType, int);
  Element* element_first_get(Element&, ElementType);
  const Element* element_first_get(const Element&, ElementType);
  Element* animation_item_get(Element&, ItemType, int = -1);
  const Element* animation_item_get(const Element&, ItemType, int = -1);
  ElementType item_type_to_track_type_get(ItemType);
  Element* track_frame_get(Element&, int);
  const Element* track_frame_get(const Element&, int);
  Element frame_generate(const Element&, float);
  int frame_index_from_at_frame_get(const Element&, int);
  int frame_index_from_time_get(const Element&, float);
  float frame_time_from_index_get(const Element&, int);
  void frame_bake(Element&, int, int, bool, bool);
  void frames_generate_from_grid(Element&, glm::ivec2, glm::ivec2, glm::ivec2, int, int, int);
  bool frames_deserialize(Element&, const std::string&, int, std::set<int>&, std::string* = nullptr);
  void frames_sort_by_at_frame(Element&);
  void frames_change(Element&, FrameChange, ItemType, ChangeType, const std::set<int>&);
  int track_length_get(const Element&);
  int animation_length_get(const Element&);

  class Anm2
  {
  public:
    bool isValid{true};
    Element root{};

    Anm2();
    Anm2(const std::filesystem::path&, std::string* = nullptr);

    bool load(const std::filesystem::path&, std::string* = nullptr);
    bool load_string(std::string_view, std::string* = nullptr);
    bool save(const std::filesystem::path&, std::string* = nullptr, Options = {}) const;
    std::string to_string(Options = {}) const;
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, Options = {}) const;
    std::uint64_t hash(Options = {}) const;
    bool is_special_interpolated_frames() const;
    void special_interpolated_frames_bake(int, bool, bool);
    void region_frames_sync(bool);
    Anm2 normalized_for_serialize() const;

    Element* element_get(ElementType);
    const Element* element_get(ElementType) const;
    Element* element_get(ElementType, int);
    const Element* element_get(ElementType, int) const;
    Element* element_get(int, ItemType, int = -1);
    const Element* element_get(int, ItemType, int = -1) const;
    Element* element_get(int, ItemType, int, int);
    const Element* element_get(int, ItemType, int, int) const;
    std::set<int> element_unused(ElementType) const;
    std::set<int> element_unused(ElementType, const Element&) const;
    std::set<int> element_unused(ElementType, int) const;
    bool deserialize(ElementType, const std::string&, bool, std::string* = nullptr, const std::filesystem::path& = {});
    bool regions_deserialize(int, const std::string&, bool, std::string* = nullptr);
    Element frame_effective(int, const Element&) const;
    glm::vec4 animation_rect(const Element&, bool) const;
    int layer_animation_add(int, int = -1, int = -1, std::string = {}, int = 0,
                            types::destination::Type = types::destination::ALL);
    int null_animation_add(int, int = -1, std::string = {}, bool = false,
                           types::destination::Type = types::destination::ALL);
    bool animations_deserialize(const std::string&, int, std::set<int>&, std::string* = nullptr);
    int animations_merge(int, std::set<int>&, types::merge::Type = types::merge::APPEND, bool = true);
  };
}
