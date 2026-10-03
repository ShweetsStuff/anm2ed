#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <tinyxml2/tinyxml2.h>

#include "model/common.hpp"

namespace anm2ed
{
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
    std::vector<std::pair<std::string, std::string>> extraAttributes{};
    std::string text{};
    glm::vec2 pivot{};
    glm::vec2 crop{};
    glm::vec2 position{};
    glm::vec2 size{};
    glm::vec2 scale{100.0f, 100.0f};
    glm::vec2 shear{};
    glm::vec3 colorOffset{};
    glm::vec4 tint{types::color::WHITE};
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
  Element& child_ensure(Element&, ElementType);
  Element element_read(const tinyxml2::XMLElement*);
  std::string element_to_string(const Element&, Flags = SERIALIZE_EDITOR_DEFAULT);
  std::string element_to_string(const Element&, ElementType, Flags = SERIALIZE_EDITOR_DEFAULT);
  tinyxml2::XMLElement* element_to_xml(tinyxml2::XMLDocument&, const Element&, ElementType,
                                       Flags = SERIALIZE_EDITOR_DEFAULT);
  Element root_animation_make();
  void frame_mix(Element&, const Element&, float);
  Element frame_generate(const Element&, float);
  int track_length_get(const Element&);

  class Anm2
  {
  public:
    bool isValid{true};
    Element root{};

    Anm2();

    bool load(const std::filesystem::path&, std::string* = nullptr);
    bool load_string(std::string_view, std::string* = nullptr);
    bool save(const std::filesystem::path&, std::string* = nullptr, Options = {}) const;
    std::string to_string(Options = {}) const;
    tinyxml2::XMLElement* to_element(tinyxml2::XMLDocument&, Options = {}) const;
    void region_frames_sync(bool);
    Anm2 normalized_for_serialize(Flags = SERIALIZE_DEFAULT) const;
  };

}
