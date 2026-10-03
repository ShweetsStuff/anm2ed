#include "internal.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
  constexpr std::array<std::string_view, (std::size_t)ElementType::COUNT> ELEMENT_TAGS = {
#define X(symbol, tag) std::string_view{tag},
      ANM2_ELEMENT_TYPES
#undef X
  };


  ElementType element_type_get(std::string_view tag)
  {
    for (std::size_t i = 0; i < ELEMENT_TAGS.size(); ++i)
      if (ELEMENT_TAGS[i] == tag) return (ElementType)i;
    return ElementType::UNKNOWN;
  }

  std::string_view element_tag_get(ElementType type)
  {
    auto index = (std::size_t)type;
    if (index >= ELEMENT_TAGS.size()) return {};
    return ELEMENT_TAGS[index];
  }

  ElementType element_container_type_get(ElementType type)
  {
    switch (type)
    {
      case ElementType::SPRITESHEET:
        return ElementType::SPRITESHEETS;
      case ElementType::SHADER:
        return ElementType::SHADERS;
      case ElementType::LAYER_ELEMENT:
        return ElementType::LAYERS;
      case ElementType::NULL_ELEMENT:
        return ElementType::NULLS;
      case ElementType::EVENT_ELEMENT:
        return ElementType::EVENTS;
      case ElementType::SOUND_ELEMENT:
        return ElementType::SOUNDS;
      case ElementType::ANIMATION:
        return ElementType::ANIMATIONS;
      default:
        return ElementType::UNKNOWN;
    }
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

  Element* child_first_get(Element& element, ElementType type)
  {
    for (auto& child : element.children)
      if (child.type == type) return &child;
    return nullptr;
  }

  const Element* child_first_get(const Element& element, ElementType type)
  {
    for (const auto& child : element.children)
      if (child.type == type) return &child;
    return nullptr;
  }

  Element* element_child_first_get(Element& element, ElementType type) { return child_first_get(element, type); }

  const Element* element_child_first_get(const Element& element, ElementType type)
  {
    return child_first_get(element, type);
  }

  Element* shader_uniform_get(Element& shader, std::string_view name, bool isCreate)
  {
    for (auto& child : shader.children)
      if (child.type == ElementType::UNIFORM && child.name == name) return &child;
    if (!isCreate) return nullptr;

    shader.children.push_back(element_make(ElementType::UNIFORM));
    shader.children.back().name = std::string(name);
    return &shader.children.back();
  }

  const Element* shader_uniform_get(const Element& shader, std::string_view name)
  {
    for (const auto& child : shader.children)
      if (child.type == ElementType::UNIFORM && child.name == name) return &child;
    return nullptr;
  }

  Element* shader_uniform_component_get(Element& uniform, int index, bool isCreate)
  {
    for (auto& child : uniform.children)
      if (child.type == ElementType::COMPONENT && child.index == index) return &child;
    if (!isCreate) return nullptr;

    uniform.children.push_back(element_make(ElementType::COMPONENT));
    uniform.children.back().index = index;
    return &uniform.children.back();
  }

  const Element* shader_uniform_component_get(const Element& uniform, int index)
  {
    for (const auto& child : uniform.children)
      if (child.type == ElementType::COMPONENT && child.index == index) return &child;
    return nullptr;
  }

  Element* child_id_get(Element& element, ElementType type, int id)
  {
    for (auto& child : element.children)
      if (child.type == type && child.id == id) return &child;
    return nullptr;
  }

  const Element* child_id_get(const Element& element, ElementType type, int id)
  {
    for (const auto& child : element.children)
      if (child.type == type && child.id == id) return &child;
    return nullptr;
  }

  Element* element_child_id_get(Element& element, ElementType type, int id) { return child_id_get(element, type, id); }

  const Element* element_child_id_get(const Element& element, ElementType type, int id)
  {
    return child_id_get(element, type, id);
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
    for (auto it = element.children.begin(); it != element.children.end(); ++it)
    {
      if (it->type != type || it->id != id) continue;
      element.children.erase(it);
      return true;
    }
    return false;
  }

  Element* element_first_get(Element& element, ElementType type)
  {
    if (element.type == type) return &element;
    for (auto& child : element.children)
      if (auto result = element_first_get(child, type); result) return result;
    return nullptr;
  }

  const Element* element_first_get(const Element& element, ElementType type)
  {
    if (element.type == type) return &element;
    for (const auto& child : element.children)
      if (auto result = element_first_get(child, type); result) return result;
    return nullptr;
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

  bool is_track(const Element& element)
  {
    return element.type == ElementType::ROOT_ANIMATION || element.type == ElementType::LAYER_ANIMATION ||
           element.type == ElementType::NULL_ANIMATION || element.type == ElementType::TRIGGERS;
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

  ElementType track_frame_type_get(const Element& track)
  {
    return track.type == ElementType::TRIGGERS ? ElementType::TRIGGER : ElementType::FRAME;
  }

  ElementType item_type_to_track_type_get(ItemType type)
  {
    if (type == ItemType::ROOT) return ElementType::ROOT_ANIMATION;
    if (type == ItemType::LAYER) return ElementType::LAYER_ANIMATION;
    if (type == ItemType::NULL_) return ElementType::NULL_ANIMATION;
    if (type == ItemType::TRIGGER) return ElementType::TRIGGERS;
    return ElementType::UNKNOWN;
  }

  ElementType item_type_to_container_type_get(ItemType type)
  {
    if (type == ItemType::LAYER) return ElementType::LAYER_ANIMATIONS;
    if (type == ItemType::NULL_) return ElementType::NULL_ANIMATIONS;
    return ElementType::UNKNOWN;
  }

  Element* animation_group_get(Element& animation, int groupType, int groupId)
  {
    if (groupId < 0) return nullptr;
    auto containerType = item_type_to_container_type_get(static_cast<ItemType>(groupType));
    auto container = child_first_get(animation, containerType);
    return container ? child_id_get(*container, ElementType::GROUP, groupId) : nullptr;
  }

  const Element* animation_group_get(const Element& animation, int groupType, int groupId)
  {
    if (groupId < 0) return nullptr;
    auto containerType = item_type_to_container_type_get(static_cast<ItemType>(groupType));
    auto container = child_first_get(animation, containerType);
    return container ? child_id_get(*container, ElementType::GROUP, groupId) : nullptr;
  }

  Element* animation_group_root_get(Element& animation, int groupType, int groupId)
  {
    auto group = animation_group_get(animation, groupType, groupId);
    return group ? child_first_get(*group, ElementType::ROOT_ANIMATION) : nullptr;
  }

  const Element* animation_group_root_get(const Element& animation, int groupType, int groupId)
  {
    auto group = animation_group_get(animation, groupType, groupId);
    return group ? child_first_get(*group, ElementType::ROOT_ANIMATION) : nullptr;
  }

  Element* animation_group_root_ensure(Element& animation, int groupType, int groupId)
  {
    auto group = animation_group_get(animation, groupType, groupId);
    if (!group) return nullptr;
    if (auto root = child_first_get(*group, ElementType::ROOT_ANIMATION)) return root;
    group->children.push_back(root_animation_make());
    return &group->children.back();
  }

  int track_id_get(const Element& track)
  {
    if (track.type == ElementType::LAYER_ANIMATION) return track.layerId;
    if (track.type == ElementType::NULL_ANIMATION) return track.nullId;
    return -1;
  }

  Element* track_find(Element& parent, ElementType trackType, int id)
  {
    for (auto& child : parent.children)
    {
      if (child.type == trackType && track_id_get(child) == id) return &child;
      if (child.type == ElementType::GROUP)
        if (auto found = track_find(child, trackType, id)) return found;
    }
    return nullptr;
  }

  const Element* track_find(const Element& parent, ElementType trackType, int id)
  {
    for (const auto& child : parent.children)
    {
      if (child.type == trackType && track_id_get(child) == id) return &child;
      if (child.type == ElementType::GROUP)
        if (auto found = track_find(child, trackType, id)) return found;
    }
    return nullptr;
  }

  Element* track_group_find(Element& parent, ElementType trackType, int id, int groupId)
  {
    for (auto& child : parent.children)
    {
      if (child.type == trackType && track_id_get(child) == id && child.groupId == groupId) return &child;
      if (child.type == ElementType::GROUP)
        if (auto found = track_group_find(child, trackType, id, groupId)) return found;
    }
    return nullptr;
  }

  const Element* track_group_find(const Element& parent, ElementType trackType, int id, int groupId)
  {
    for (const auto& child : parent.children)
    {
      if (child.type == trackType && track_id_get(child) == id && child.groupId == groupId) return &child;
      if (child.type == ElementType::GROUP)
        if (auto found = track_group_find(child, trackType, id, groupId)) return found;
    }
    return nullptr;
  }

  bool is_track_group_visible(const Element& container, const Element& track)
  {
    if (track.groupId == -1) return true;
    for (const auto& child : container.children)
      if (child.type == ElementType::GROUP && child.id == track.groupId) return child.isVisible;
    return true;
  }

  Element* animation_item_get(Element& animation, ItemType type, int id, int groupType, int groupId)
  {
    auto trackType = item_type_to_track_type_get(type);
    if (type == ItemType::ROOT && groupType != NONE && groupId != -1)
      return animation_group_root_get(animation, groupType, groupId);
    if (type == ItemType::ROOT || type == ItemType::TRIGGER) return child_first_get(animation, trackType);

    auto container = child_first_get(animation, item_type_to_container_type_get(type));
    if (!container) return nullptr;
    return groupType != NONE && groupId != -1 ? track_group_find(*container, trackType, id, groupId)
                                              : track_find(*container, trackType, id);
  }

  Element* animation_item_get(Element& animation, ItemType type, int id)
  {
    return animation_item_get(animation, type, id, NONE, -1);
  }

  const Element* animation_item_get(const Element& animation, ItemType type, int id, int groupType, int groupId)
  {
    auto trackType = item_type_to_track_type_get(type);
    if (type == ItemType::ROOT && groupType != NONE && groupId != -1)
      return animation_group_root_get(animation, groupType, groupId);
    if (type == ItemType::ROOT || type == ItemType::TRIGGER) return child_first_get(animation, trackType);

    auto container = child_first_get(animation, item_type_to_container_type_get(type));
    if (!container) return nullptr;
    return groupType != NONE && groupId != -1 ? track_group_find(*container, trackType, id, groupId)
                                              : track_find(*container, trackType, id);
  }

  const Element* animation_item_get(const Element& animation, ItemType type, int id)
  {
    return animation_item_get(animation, type, id, NONE, -1);
  }

  int track_frame_child_index_get(const Element& track, int index)
  {
    if (index < 0) return -1;
    auto frameType = track_frame_type_get(track);
    int frameIndex{};
    for (int i = 0; i < (int)track.children.size(); ++i)
    {
      if (track.children[i].type != frameType) continue;
      if (frameIndex == index) return i;
      ++frameIndex;
    }
    return -1;
  }

  int track_frames_count_get(const Element& track)
  {
    auto frameType = track_frame_type_get(track);
    int count{};
    for (const auto& child : track.children)
      if (child.type == frameType) ++count;
    return count;
  }

  int track_frame_insert_child_index_get(const Element& track, int index)
  {
    auto targetFrameIndex = std::max(0, index);
    auto frameType = track_frame_type_get(track);
    int frameIndex{};
    for (int i = 0; i < (int)track.children.size(); ++i)
    {
      if (track.children[i].type != frameType) continue;
      if (frameIndex == targetFrameIndex) return i;
      ++frameIndex;
    }
    return (int)track.children.size();
  }

  Element* track_frame_get(Element& track, int index)
  {
    auto childIndex = track_frame_child_index_get(track, index);
    return childIndex == -1 ? nullptr : &track.children[childIndex];
  }

  const Element* track_frame_get(const Element& track, int index)
  {
    auto childIndex = track_frame_child_index_get(track, index);
    return childIndex == -1 ? nullptr : &track.children[childIndex];
  }

  Element* Anm2::element_get(ElementType type)
  {
    if (type == ElementType::ANIMATED_ACTOR) return &root;
    if (type == ElementType::ANIMATIONS) return element_first_get(root, type);

    auto content = child_first_get(root, ElementType::CONTENT);
    if (!content) return nullptr;
    if (type == ElementType::CONTENT) return content;
    if (auto container = child_first_get(*content, type)) return container;

    switch (type)
    {
      case ElementType::SPRITESHEETS:
      case ElementType::SHADERS:
      case ElementType::LAYERS:
      case ElementType::NULLS:
      case ElementType::EVENTS:
      case ElementType::SOUNDS:
        content->children.push_back(element_make(type));
        return &content->children.back();
      default:
        break;
    }

    return element_first_get(root, type);
  }

  const Element* Anm2::element_get(ElementType type) const
  {
    if (type == ElementType::ANIMATED_ACTOR) return &root;
    if (type == ElementType::ANIMATIONS) return element_first_get(root, type);

    auto content = child_first_get(root, ElementType::CONTENT);
    if (!content) return nullptr;
    if (type == ElementType::CONTENT) return content;
    if (auto container = child_first_get(*content, type)) return container;
    return element_first_get(root, type);
  }

  Element* Anm2::element_get(ElementType type, int id)
  {
    if (type == ElementType::ANIMATION)
    {
      auto animations = element_get(ElementType::ANIMATIONS);
      if (!animations || id < 0) return nullptr;
      int current{};
      for (auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        if (current == id) return &animation;
        ++current;
      }
      return nullptr;
    }

    auto container = element_get(element_container_type_get(type));
    return container ? child_id_get(*container, type, id) : nullptr;
  }

  const Element* Anm2::element_get(ElementType type, int id) const
  {
    if (type == ElementType::ANIMATION)
    {
      auto animations = element_get(ElementType::ANIMATIONS);
      if (!animations || id < 0) return nullptr;
      int current{};
      for (const auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        if (current == id) return &animation;
        ++current;
      }
      return nullptr;
    }

    auto container = element_get(element_container_type_get(type));
    return container ? child_id_get(*container, type, id) : nullptr;
  }

  Element* Anm2::element_get(int animationIndex, ItemType type, int id)
  {
    auto animation = element_get(ElementType::ANIMATION, animationIndex);
    return animation ? animation_item_get(*animation, type, id) : nullptr;
  }

  const Element* Anm2::element_get(int animationIndex, ItemType type, int id) const
  {
    auto animation = element_get(ElementType::ANIMATION, animationIndex);
    return animation ? animation_item_get(*animation, type, id) : nullptr;
  }

  Element* Anm2::element_get(int animationIndex, ItemType type, int id, int groupType, int groupId)
  {
    auto animation = element_get(ElementType::ANIMATION, animationIndex);
    return animation ? animation_item_get(*animation, type, id, groupType, groupId) : nullptr;
  }

  const Element* Anm2::element_get(int animationIndex, ItemType type, int id, int groupType, int groupId) const
  {
    auto animation = element_get(ElementType::ANIMATION, animationIndex);
    return animation ? animation_item_get(*animation, type, id, groupType, groupId) : nullptr;
  }

  Element* Anm2::element_get(int animationIndex, ItemType type, int frameIndex, int id)
  {
    auto item = element_get(animationIndex, type, id);
    return item ? track_frame_get(*item, frameIndex) : nullptr;
  }

  const Element* Anm2::element_get(int animationIndex, ItemType type, int frameIndex, int id) const
  {
    auto item = element_get(animationIndex, type, id);
    return item ? track_frame_get(*item, frameIndex) : nullptr;
  }

  Element* Anm2::element_get(Reference reference)
  {
    auto itemType = static_cast<ItemType>(reference.itemType);
    auto item =
        element_get(reference.animationIndex, itemType, reference.itemID, reference.groupType, reference.groupId);
    if (reference.frameIndex < 0) return item;
    return item ? track_frame_get(*item, reference.frameIndex) : nullptr;
  }

  const Element* Anm2::element_get(Reference reference) const
  {
    auto itemType = static_cast<ItemType>(reference.itemType);
    auto item =
        element_get(reference.animationIndex, itemType, reference.itemID, reference.groupType, reference.groupId);
    if (reference.frameIndex < 0) return item;
    return item ? track_frame_get(*item, reference.frameIndex) : nullptr;
  }

  std::set<int> Anm2::element_unused(ElementType type) const
  {
    std::set<int> used{};

    if (type == ElementType::SPRITESHEET)
    {
      if (auto layers = element_get(ElementType::LAYERS))
        for (const auto& layer : layers->children)
          if (layer.type == ElementType::LAYER_ELEMENT && layer.spritesheetId != -1) used.insert(layer.spritesheetId);
    }
    else if (auto animations = element_get(ElementType::ANIMATIONS))
      for (const auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        if (type == ElementType::EVENT_ELEMENT)
        {
          auto triggers = child_first_get(animation, ElementType::TRIGGERS);
          if (!triggers) continue;
          for (const auto& trigger : triggers->children)
            if (trigger.type == ElementType::TRIGGER && trigger.eventId != -1) used.insert(trigger.eventId);
        }
        else if (type == ElementType::SOUND_ELEMENT)
        {
          auto triggers = child_first_get(animation, ElementType::TRIGGERS);
          if (!triggers) continue;
          for (const auto& trigger : triggers->children)
          {
            if (trigger.type != ElementType::TRIGGER) continue;
            for (auto id : trigger.soundIds)
              used.insert(id);
          }
        }
        else if (type == ElementType::SHADER)
        {
          auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
          if (!layerAnimations) continue;
          tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                      [&](const Element& layerAnimation)
                      {
                        for (const auto& frame : layerAnimation.children)
                          if (frame.type == ElementType::FRAME && frame.shaderId != -1) used.insert(frame.shaderId);
                      });
        }
        else if (type == ElementType::LAYER_ELEMENT || type == ElementType::NULL_ELEMENT)
        {
          auto containerType =
              type == ElementType::LAYER_ELEMENT ? ElementType::LAYER_ANIMATIONS : ElementType::NULL_ANIMATIONS;
          auto tracks = child_first_get(animation, containerType);
          if (!tracks) continue;
          auto trackType =
              type == ElementType::LAYER_ELEMENT ? ElementType::LAYER_ANIMATION : ElementType::NULL_ANIMATION;
          tracks_each(*tracks, trackType,
                      [&](const Element& track)
                      {
                        if (track.type == ElementType::LAYER_ANIMATION)
                          used.insert(track.layerId);
                        else if (track.type == ElementType::NULL_ANIMATION)
                          used.insert(track.nullId);
                      });
        }
      }

    std::set<int> unused{};
    if (auto container = element_get(element_container_type_get(type)))
      for (const auto& element : container->children)
        if (element.type == type && !used.contains(element.id)) unused.insert(element.id);
    return unused;
  }

  std::set<int> Anm2::element_unused(ElementType type, const Element& animation) const
  {
    if (type != ElementType::LAYER_ELEMENT && type != ElementType::NULL_ELEMENT) return {};

    std::set<int> used{};
    auto containerType =
        type == ElementType::LAYER_ELEMENT ? ElementType::LAYER_ANIMATIONS : ElementType::NULL_ANIMATIONS;
    if (auto tracks = child_first_get(animation, containerType))
    {
      auto trackType = type == ElementType::LAYER_ELEMENT ? ElementType::LAYER_ANIMATION : ElementType::NULL_ANIMATION;
      tracks_each(*tracks, trackType,
                  [&](const Element& track)
                  {
                    if (track.type == ElementType::LAYER_ANIMATION)
                      used.insert(track.layerId);
                    else if (track.type == ElementType::NULL_ANIMATION)
                      used.insert(track.nullId);
                  });
    }

    std::set<int> unused{};
    if (auto container = element_get(element_container_type_get(type)))
      for (const auto& element : container->children)
        if (element.type == type && !used.contains(element.id)) unused.insert(element.id);
    return unused;
  }

  std::set<int> Anm2::element_unused(ElementType type, int parentId) const
  {
    if (type != ElementType::REGION) return {};

    std::set<int> used{};
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (animations)
      for (const auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
        if (!layerAnimations) continue;
        tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                    [&](const Element& layerAnimation)
                    {
                      auto layer = element_get(ElementType::LAYER_ELEMENT, layerAnimation.layerId);
                      if (!layer || layer->spritesheetId != parentId) return;
                      for (const auto& frame : layerAnimation.children)
                      {
                        if (frame.type != ElementType::FRAME) continue;
                        if (frame.regionId != -1) used.insert(frame.regionId);
                      }
                    });
      }

    std::set<int> unused{};
    if (auto spritesheet = element_get(ElementType::SPRITESHEET, parentId))
      for (const auto& child : spritesheet->children)
        if (child.type == type && !used.contains(child.id)) unused.insert(child.id);
    return unused;
  }
}
