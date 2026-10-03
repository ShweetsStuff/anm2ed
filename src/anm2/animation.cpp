#include "internal.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
  Element Anm2::frame_effective(int layerId, const Element& frame) const
  {
    auto resolved = frame;
    if (frame.regionId == -1) return resolved;

    auto layer = element_get(ElementType::LAYER_ELEMENT, layerId);
    if (!layer) return resolved;

    auto spritesheet = element_get(ElementType::SPRITESHEET, layer->spritesheetId);
    if (!spritesheet) return resolved;

    auto region = element_child_id_get(*spritesheet, ElementType::REGION, frame.regionId);
    if (!region) return resolved;

    resolved.crop = region->crop;
    resolved.size = region->size;
    resolved.pivot = region->pivot;
    return resolved;
  }

  glm::vec4 Anm2::animation_rect(const Element& animation, bool isRootTransform) const
  {
    constexpr glm::ivec2 CORNERS[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    float minX = std::numeric_limits<float>::infinity();
    float minY = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    float maxY = -std::numeric_limits<float>::infinity();
    bool isAny{};

    for (float t = 0.0f; t < (float)animation.frameNum; t += 1.0f)
    {
      glm::mat4 transform(1.0f);

      auto root = animation_item_get(animation, ItemType::ROOT);
      if (isRootTransform && root)
      {
        auto rootFrame = frame_generate(*root, t);
        transform *= math::quad_model_parent_get(rootFrame.position, {}, math::percent_to_unit(rootFrame.scale),
                                                 rootFrame.rotation, math::percent_to_unit(rootFrame.shear));
      }

      auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
      if (!layerAnimations) continue;

      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](const Element& layerAnimation)
                  {
                    if (!layerAnimation.isVisible || !is_track_group_visible(*layerAnimations, layerAnimation)) return;

                    auto itemTransform = transform;
                    if (isRootTransform && layerAnimation.groupId != -1)
                      if (auto group = child_id_get(*layerAnimations, ElementType::GROUP, layerAnimation.groupId))
                        if (auto groupRoot = child_first_get(*group, ElementType::ROOT_ANIMATION))
                        {
                          auto groupRootFrame = frame_generate(*groupRoot, t);
                          itemTransform *= math::quad_model_parent_get(
                              groupRootFrame.position, {}, math::percent_to_unit(groupRootFrame.scale),
                              groupRootFrame.rotation, math::percent_to_unit(groupRootFrame.shear));
                        }

                    auto frame = frame_effective(layerAnimation.layerId, frame_generate(layerAnimation, t));
                    if (frame.size == glm::vec2() || !frame.isVisible) return;

                    auto layerTransform =
                        itemTransform * math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                             math::percent_to_unit(frame.scale), frame.rotation,
                                                             math::percent_to_unit(frame.shear));
                    for (auto& corner : CORNERS)
                    {
                      auto world = layerTransform * glm::vec4(corner, 0.0f, 1.0f);
                      minX = std::min(minX, world.x);
                      minY = std::min(minY, world.y);
                      maxX = std::max(maxX, world.x);
                      maxY = std::max(maxY, world.y);
                      isAny = true;
                    }
                  });
    }

    if (!isAny) return glm::vec4(-1.0f);
    return {minX, minY, maxX - minX, maxY - minY};
  }

  Element* animation_container_get(Element& animation, ElementType type)
  {
    if (auto container = child_first_get(animation, type)) return container;
    animation.children.push_back(element_make(type));
    return &animation.children.back();
  }

  int Anm2::layer_animation_add(int animationIndex, int id, int insertBeforeId, std::string name, int spritesheetId,
                                types::destination::Type destination)
  {
    auto layers = element_get(ElementType::LAYERS);
    if (!layers) return -1;

    id = id == -1 ? element_child_next_id_get(*layers, ElementType::LAYER_ELEMENT) : id;
    auto layer = element_child_id_get(*layers, ElementType::LAYER_ELEMENT, id);
    if (!layer)
    {
      layers->children.push_back(element_make(ElementType::LAYER_ELEMENT));
      layer = &layers->children.back();
      layer->id = id;
    }

    if (!name.empty()) layer->name = name;
    layer->spritesheetId = element_get(ElementType::SPRITESHEET, spritesheetId) ? spritesheetId : 0;

    auto add = [&](Element& animation)
    {
      if (animation_item_get(animation, ItemType::LAYER, id)) return;
      auto layerAnimations = animation_container_get(animation, ElementType::LAYER_ANIMATIONS);
      auto item = element_make(ElementType::LAYER_ANIMATION);
      item.layerId = id;

      if (insertBeforeId != -1)
        for (auto it = layerAnimations->children.begin(); it != layerAnimations->children.end(); ++it)
          if (it->type == ElementType::LAYER_ANIMATION && it->layerId == insertBeforeId)
          {
            layerAnimations->children.insert(it, item);
            return;
          }

      layerAnimations->children.push_back(item);
    };

    if (destination == types::destination::ALL)
    {
      if (auto animations = element_get(ElementType::ANIMATIONS))
        for (auto& animation : animations->children)
          if (animation.type == ElementType::ANIMATION) add(animation);
    }
    else if (auto animation = element_get(ElementType::ANIMATION, animationIndex))
      add(*animation);

    return id;
  }

  int Anm2::null_animation_add(int animationIndex, int id, std::string name, bool isShowRect,
                               types::destination::Type destination)
  {
    auto nulls = element_get(ElementType::NULLS);
    if (!nulls) return -1;

    id = id == -1 ? element_child_next_id_get(*nulls, ElementType::NULL_ELEMENT) : id;
    auto null = element_child_id_get(*nulls, ElementType::NULL_ELEMENT, id);
    if (!null)
    {
      nulls->children.push_back(element_make(ElementType::NULL_ELEMENT));
      null = &nulls->children.back();
      null->id = id;
    }

    if (!name.empty()) null->name = name;
    null->isShowRect = isShowRect;

    auto add = [&](Element& animation)
    {
      if (animation_item_get(animation, ItemType::NULL_, id)) return;
      auto nullAnimations = animation_container_get(animation, ElementType::NULL_ANIMATIONS);
      auto item = element_make(ElementType::NULL_ANIMATION);
      item.nullId = id;
      nullAnimations->children.push_back(item);
    };

    if (destination == types::destination::ALL)
    {
      if (auto animations = element_get(ElementType::ANIMATIONS))
        for (auto& animation : animations->children)
          if (animation.type == ElementType::ANIMATION) add(animation);
    }
    else if (auto animation = element_get(ElementType::ANIMATION, animationIndex))
      add(*animation);

    return id;
  }
}
