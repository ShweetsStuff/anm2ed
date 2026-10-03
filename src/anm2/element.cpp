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

}
