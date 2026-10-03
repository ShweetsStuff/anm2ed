#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <glm/glm.hpp>
#include <tinyxml2/tinyxml2.h>

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

  struct FrameChange
  {
    std::optional<bool> isVisible{};
    std::optional<Interpolation> interpolation{};
    std::optional<int> shaderId{};
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
    std::optional<float> shearX{};
    std::optional<float> shearY{};
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
    std::filesystem::path vertex{};
    std::filesystem::path fragment{};
    std::string binding{};
    std::string value{};
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
    int shaderId{-1};
    int soundId{-1};
    int groupId{-1};
    int index{-1};
    bool isLoop{true};
    bool isVisible{true};
    bool isShowRect{};
    bool isExpanded{true};
    bool isEnabled{true};
    Interpolation interpolation{Interpolation::NONE};
    Origin origin{Origin::CUSTOM};
    float rotation{};
    std::vector<int> soundIds{};
    glm::vec2 pivot{};
    glm::vec2 crop{};
    glm::vec2 position{};
    glm::vec2 size{};
    glm::vec2 scale{100.0f, 100.0f};
    glm::vec2 shear{};
    glm::vec3 colorOffset{};
    glm::vec4 tint{types::color::WHITE};
  };

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

  struct TrackContainer
  {
    ItemType itemType;
    ElementType container;
    ElementType track;
    ElementType groups;
    ElementType elements;
    ElementType element;
    int Element::* id;
  };

  inline constexpr TrackContainer TRACK_CONTAINERS[] = {
      {ItemType::LAYER, ElementType::LAYER_ANIMATIONS, ElementType::LAYER_ANIMATION,
       ElementType::LAYER_ANIMATION_GROUPS, ElementType::LAYERS, ElementType::LAYER_ELEMENT, &Element::layerId},
      {ItemType::NULL_, ElementType::NULL_ANIMATIONS, ElementType::NULL_ANIMATION, ElementType::NULL_ANIMATION_GROUPS,
       ElementType::NULLS, ElementType::NULL_ELEMENT, &Element::nullId}};

  inline constexpr int GROUP_ANY = -2;

  template <class E> using ElementPointer = std::conditional_t<std::is_const_v<E>, const Element*, Element*>;

  constexpr const TrackContainer* track_container_get(ElementType type)
  {
    for (const auto& row : TRACK_CONTAINERS)
      if (type == row.container || type == row.track || type == row.groups || type == row.elements ||
          type == row.element)
        return &row;
    return nullptr;
  }

  constexpr const TrackContainer* track_container_get(ItemType type)
  {
    for (const auto& row : TRACK_CONTAINERS)
      if (type == row.itemType) return &row;
    return nullptr;
  }

  constexpr bool is_track(const Element& element)
  {
    return element.type != ElementType::UNKNOWN && std::ranges::contains(TYPE_TRACKS, element.type);
  }

  constexpr ElementType track_frame_type_get(const Element& track)
  {
    return track.type == ElementType::TRIGGERS ? ElementType::TRIGGER : ElementType::FRAME;
  }

  constexpr int track_id_get(const Element& track)
  {
    auto row = track_container_get(track.type);
    return row && row->track == track.type ? track.*(row->id) : -1;
  }

  template <class E, class Predicate> ElementPointer<E> child_find(E& element, Predicate&& isMatch)
  {
    for (auto& child : element.children)
      if (isMatch(child)) return &child;
    return nullptr;
  }

  template <class E> ElementPointer<E> child_first_get(E& element, ElementType type)
  {
    return child_find(element, [&](const Element& child) { return child.type == type; });
  }

  template <class E> ElementPointer<E> child_id_get(E& element, ElementType type, int id)
  {
    return child_find(element, [&](const Element& child) { return child.type == type && child.id == id; });
  }

  template <class E> ElementPointer<E> shader_uniform_get(E& shader, std::string_view name)
  {
    return child_find(shader,
                      [&](const Element& child) { return child.type == ElementType::UNIFORM && child.name == name; });
  }

  template <class E> ElementPointer<E> shader_uniform_component_get(E& uniform, int index)
  {
    return child_find(uniform, [&](const Element& child)
                      { return child.type == ElementType::COMPONENT && child.index == index; });
  }

  template <class E> ElementPointer<E> element_first_get(E& element, ElementType type)
  {
    if (element.type == type) return &element;
    for (auto& child : element.children)
      if (auto result = element_first_get(child, type)) return result;
    return nullptr;
  }

  template <class E> ElementPointer<E> track_find(E& parent, ElementType trackType, int id, int groupId = GROUP_ANY)
  {
    for (auto& child : parent.children)
    {
      if (child.type == trackType && track_id_get(child) == id && (groupId == GROUP_ANY || child.groupId == groupId))
        return &child;
      if (child.type == ElementType::GROUP)
        if (auto found = track_find(child, trackType, id, groupId)) return found;
    }
    return nullptr;
  }

  template <class E> ElementPointer<E> animation_group_get(E& animation, int groupType, int groupId)
  {
    auto container = groupId < 0 ? nullptr : child_first_get(animation, TYPE_CONTAINERS[groupType]);
    return container ? child_id_get(*container, ElementType::GROUP, groupId) : nullptr;
  }

  template <class E>
  ElementPointer<E> animation_item_get(E& animation, ItemType type, int id = -1, int groupType = NONE, int groupId = -1)
  {
    auto trackType = TYPE_TRACKS[(int)type];
    auto isGrouped = groupType != NONE && groupId != -1;
    if (type == ItemType::ROOT && isGrouped)
    {
      auto group = animation_group_get(animation, groupType, groupId);
      return group ? child_first_get(*group, ElementType::ROOT_ANIMATION) : nullptr;
    }
    if (type == ItemType::ROOT || type == ItemType::TRIGGER) return child_first_get(animation, trackType);

    auto container = child_first_get(animation, TYPE_CONTAINERS[(int)type]);
    return container ? track_find(*container, trackType, id, isGrouped ? groupId : GROUP_ANY) : nullptr;
  }

  int track_frame_child_index_get(const Element&, int);
  int track_frame_insert_child_index_get(const Element&, int);

  template <class E> ElementPointer<E> track_frame_get(E& track, int index)
  {
    auto childIndex = track_frame_child_index_get(track, index);
    return childIndex == -1 ? nullptr : &track.children[childIndex];
  }

  template <class E, class Callback> void tracks_each(E& parent, ElementType trackType, Callback&& callback)
  {
    for (auto& child : parent.children)
      if (child.type == trackType)
        callback(child);
      else if (child.type == ElementType::GROUP)
        tracks_each(child, trackType, callback);
  }

  template <class E, class Callback> void animations_tracks_each(E& root, ElementType trackType, Callback&& callback)
  {
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    auto row = track_container_get(trackType);
    if (!animations || !row) return;
    for (auto& animation : animations->children)
      if (animation.type == ElementType::ANIMATION)
        if (auto container = child_first_get(animation, row->container)) tracks_each(*container, trackType, callback);
  }

  template <class E, class Callback> void element_each(E& element, Callback&& callback)
  {
    callback(element);
    for (auto& child : element.children)
      element_each(child, callback);
  }

  Element element_make(ElementType);
  Element element_read(const tinyxml2::XMLElement*);
  std::string element_to_string(const Element&, Flags = SERIALIZE_EDITOR_DEFAULT);
  std::string element_to_string(const Element&, ElementType, Flags = SERIALIZE_EDITOR_DEFAULT);
  tinyxml2::XMLElement* element_to_xml(tinyxml2::XMLDocument&, const Element&, Flags = SERIALIZE_EDITOR_DEFAULT);
  tinyxml2::XMLElement* element_to_xml(tinyxml2::XMLDocument&, const Element&, ElementType,
                                       Flags = SERIALIZE_EDITOR_DEFAULT);
  Element& child_ensure(Element&, ElementType);
  int element_child_next_id_get(const Element&, ElementType);
  int element_child_max_id_get(const Element&, ElementType);
  bool element_child_id_erase(Element&, ElementType, int);
  int track_frames_count_get(const Element&);
  int animations_count_get(const Element&);
  float interpolation_factor(Interpolation, float);
  void frame_mix(Element&, const Element&, float);
  Element frame_generate(const Element&, float);
  int frame_index_from_at_frame_get(const Element&, int);
  int frame_index_from_time_get(const Element&, float);
  float frame_time_from_index_get(const Element&, int);
  void frame_bake(Element&, int, int, bool, bool);
  void frames_generate_from_grid(Element&, glm::ivec2, glm::ivec2, glm::vec2, int, int, int);
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
    Anm2 normalized_for_serialize(Flags = SERIALIZE_DEFAULT) const;

    template <class Self> ElementPointer<Self> element_get(this Self&, ElementType);
    template <class Self> ElementPointer<Self> element_get(this Self&, ElementType, int);
    template <class Self> ElementPointer<Self> element_get(this Self&, Reference);
    std::set<int> element_unused(ElementType, const Element* = nullptr) const;
    std::set<int> region_unused(int) const;
    bool deserialize(ElementType, const std::string&, bool, std::string* = nullptr, const std::filesystem::path& = {},
                     int = -1);
    Element frame_effective(int, const Element&) const;
    glm::vec4 animation_rect(const Element&, bool) const;
    int item_add(ItemType, int, const Element&, int = -1, types::destination::Type = types::destination::ALL);
    bool animations_deserialize(const std::string&, int, std::set<int>&, std::string* = nullptr,
                                std::set<int>* = nullptr);
    int animations_merge(int, std::set<int>&, types::merge::Type = types::merge::APPEND, bool = true);
    bool file_merge(const std::filesystem::path&, const std::filesystem::path&, FileMergePreset);
    bool regions_generate(const std::set<int>&, const std::set<Reference>&, const std::string&, RegionFrameMapping);
    void regions_scan();
  };
}
