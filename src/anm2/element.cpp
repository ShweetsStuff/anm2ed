#include "internal.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
  constexpr std::array<std::string_view, (std::size_t)ElementType::COUNT> ELEMENT_TAGS = {
#define X(symbol, tag, container) std::string_view{tag},
      ANM2_ELEMENT_TYPES
#undef X
  };

  ElementType element_type_get(std::string_view tag)
  {
    auto it = std::ranges::find(ELEMENT_TAGS, tag);
    return it == ELEMENT_TAGS.end() ? ElementType::UNKNOWN : (ElementType)(it - ELEMENT_TAGS.begin());
  }

  std::string_view element_tag_get(ElementType type)
  {
    return (std::size_t)type < ELEMENT_TAGS.size() ? ELEMENT_TAGS[(std::size_t)type] : std::string_view{};
  }

  Element element_make(ElementType type)
  {
    Element element{};
    element.type = type;
    element.tag = std::string(element_tag_get(type));
    return element;
  }

  Element root_animation_make()
  {
    auto root = element_make(ElementType::ROOT_ANIMATION);
    root.children.push_back(element_make(ElementType::FRAME));
    return root;
  }

  Element& child_ensure(Element& element, ElementType type)
  {
    if (auto child = child_first_get(element, type)) return *child;
    return element.children.emplace_back(element_make(type));
  }

  Element* content_container_get(Element& root, ElementType type)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    return content ? child_first_get(*content, type) : nullptr;
  }

  std::unordered_map<int, int> child_ids_compact(Element& element, ElementType type)
  {
    std::unordered_map<int, int> remap{};
    int nextId{};
    for (auto& child : element.children)
      if (child.type == type)
      {
        remap[child.id] = nextId;
        child.id = nextId++;
      }
    return remap;
  }

  int element_child_max_id_get(const Element& element, ElementType type)
  {
    int maxId{-1};
    for (const auto& child : element.children)
      if (child.type == type) maxId = std::max(maxId, child.id);
    return maxId;
  }

  int element_child_next_id_get(const Element& element, ElementType type)
  {
    return element_child_max_id_get(element, type) + 1;
  }

  bool element_child_id_erase(Element& element, ElementType type, int id)
  {
    return std::erase_if(element.children, [&](const Element& child) { return child.type == type && child.id == id; });
  }

  float interpolation_factor(Interpolation interpolation, float value)
  {
    value = glm::clamp(value, 0.0f, 1.0f);
    if (interpolation == Interpolation::LINEAR) return value;
    if (interpolation == Interpolation::EASE_IN) return value * value;
    if (interpolation == Interpolation::EASE_OUT) return 1.0f - ((1.0f - value) * (1.0f - value));
    if (interpolation == Interpolation::EASE_IN_OUT)
      return value < 0.5f ? (2.0f * value * value) : (1.0f - std::pow(-2.0f * value + 2.0f, 2.0f) * 0.5f);
    return 0.0f;
  }

  bool is_track_child_valid(ElementType parentType, ElementType childType)
  {
    if (parentType == ElementType::CONTENT && childType == ElementType::SHADERS) return true;
    if (parentType == ElementType::SHADERS) return childType == ElementType::SHADER;
    if (parentType == ElementType::SHADER) return childType == ElementType::UNIFORM;
    if (parentType == ElementType::UNIFORM) return childType == ElementType::COMPONENT;
    if (childType == ElementType::SHADERS || childType == ElementType::SHADER || childType == ElementType::UNIFORM ||
        childType == ElementType::COMPONENT)
      return false;
    if (parentType == ElementType::TRIGGERS) return childType == ElementType::TRIGGER;
    if (parentType == ElementType::TRIGGER) return childType == ElementType::SOUND_ELEMENT;
    if (parentType == ElementType::LAYER_ANIMATION_GROUPS || parentType == ElementType::NULL_ANIMATION_GROUPS)
      return childType == ElementType::GROUP;
    if (parentType == ElementType::ROOT_ANIMATION || parentType == ElementType::LAYER_ANIMATION ||
        parentType == ElementType::NULL_ANIMATION)
      return childType == ElementType::FRAME;
    return true;
  }

  bool is_track_group_visible(const Element& container, const Element& track)
  {
    auto group = track.groupId == -1 ? nullptr : child_id_get(container, ElementType::GROUP, track.groupId);
    return !group || group->isVisible;
  }

  int track_frame_child_index_get(const Element& track, int index)
  {
    if (index < 0) return -1;
    auto frameType = track_frame_type_get(track);
    int frameIndex{};
    for (int i = 0; i < (int)track.children.size(); ++i)
      if (track.children[i].type == frameType && frameIndex++ == index) return i;
    return -1;
  }

  int track_frame_insert_child_index_get(const Element& track, int index)
  {
    auto childIndex = track_frame_child_index_get(track, std::max(0, index));
    return childIndex == -1 ? (int)track.children.size() : childIndex;
  }

  int track_frames_count_get(const Element& track)
  {
    auto frameType = track_frame_type_get(track);
    return (int)std::ranges::count_if(track.children, [&](const Element& child) { return child.type == frameType; });
  }

  template <class Self> ElementPointer<Self> Anm2::element_get(this Self& self, ElementType type)
  {
    if (type == ElementType::ANIMATED_ACTOR) return &self.root;
    if (type == ElementType::ANIMATIONS) return element_first_get(self.root, type);

    auto content = child_first_get(self.root, ElementType::CONTENT);
    if (!content) return nullptr;
    if (type == ElementType::CONTENT) return content;
    if (auto container = child_first_get(*content, type)) return container;
    if constexpr (!std::is_const_v<Self>)
      if (type != ElementType::UNKNOWN && std::ranges::contains(ELEMENT_CONTAINERS, type))
        return &content->children.emplace_back(element_make(type));
    return element_first_get(self.root, type);
  }

  template <class Self> ElementPointer<Self> Anm2::element_get(this Self& self, ElementType type, int id)
  {
    if (type != ElementType::ANIMATION)
    {
      auto container = self.element_get(ELEMENT_CONTAINERS[(int)type]);
      return container ? child_id_get(*container, type, id) : nullptr;
    }

    auto animations = self.element_get(ElementType::ANIMATIONS);
    auto childIndex = animations ? animations_child_index_get(*animations, id) : -1;
    return childIndex == -1 ? nullptr : &animations->children[childIndex];
  }

  template <class Self> ElementPointer<Self> Anm2::element_get(this Self& self, Reference reference)
  {
    auto animation = self.element_get(ElementType::ANIMATION, reference.animationIndex);
    auto item = animation ? animation_item_get(*animation, (ItemType)reference.itemType, reference.itemID,
                                               reference.groupType, reference.groupId)
                          : nullptr;
    if (reference.frameIndex < 0 || !item) return item;
    return track_frame_get(*item, reference.frameIndex);
  }

#define X(Self)                                                                                                        \
  template ElementPointer<Self> Anm2::element_get(this Self&, ElementType);                                            \
  template ElementPointer<Self> Anm2::element_get(this Self&, ElementType, int);                                       \
  template ElementPointer<Self> Anm2::element_get(this Self&, Reference);
  X(Anm2)
  X(const Anm2)
#undef X

  std::set<int> children_unused_get(const Element* element, ElementType type, const std::set<int>& used)
  {
    std::set<int> unused{};
    if (element)
      for (const auto& child : element->children)
        if (child.type == type && !used.contains(child.id)) unused.insert(child.id);
    return unused;
  }

  std::set<int> Anm2::element_unused(ElementType type, const Element* animation) const
  {
    std::set<int> used{};
    auto row = track_container_get(type);
    auto used_insert = [&](const Element& animation)
    {
      if (row)
        if (auto tracks = child_first_get(animation, row->container))
          tracks_each(*tracks, row->track, [&](const Element& track) { used.insert(track.*(row->id)); });

      if (auto triggers = child_first_get(animation, ElementType::TRIGGERS))
        for (const auto& trigger : triggers->children)
          if (trigger.type == ElementType::TRIGGER)
          {
            if (type == ElementType::EVENT_ELEMENT) used.insert(trigger.eventId);
            if (type == ElementType::SOUND_ELEMENT) used.insert(trigger.soundIds.begin(), trigger.soundIds.end());
          }

      if (type == ElementType::SHADER)
        if (auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS))
          tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                      [&](const Element& track)
                      {
                        for (const auto& frame : track.children)
                          if (frame.type == ElementType::FRAME) used.insert(frame.shaderId);
                      });
    };

    if (type == ElementType::SPRITESHEET)
    {
      if (auto layers = element_get(ElementType::LAYERS))
        for (const auto& layer : layers->children)
          if (layer.type == ElementType::LAYER_ELEMENT) used.insert(layer.spritesheetId);
    }
    else if (animation)
      used_insert(*animation);
    else if (auto animations = element_get(ElementType::ANIMATIONS))
      for (const auto& child : animations->children)
        if (child.type == ElementType::ANIMATION) used_insert(child);

    return children_unused_get(element_get(ELEMENT_CONTAINERS[(int)type]), type, used);
  }

  std::set<int> Anm2::region_unused(int spritesheetId) const
  {
    std::set<int> used{};
    animations_tracks_each(root, ElementType::LAYER_ANIMATION,
                           [&](const Element& track)
                           {
                             auto layer = element_get(ElementType::LAYER_ELEMENT, track.layerId);
                             if (!layer || layer->spritesheetId != spritesheetId) return;
                             for (const auto& frame : track.children)
                               if (frame.type == ElementType::FRAME) used.insert(frame.regionId);
                           });
    return children_unused_get(element_get(ElementType::SPRITESHEET, spritesheetId), ElementType::REGION, used);
  }
}
