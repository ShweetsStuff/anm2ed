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

    auto region = child_id_get(*spritesheet, ElementType::REGION, frame.regionId);
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

  int Anm2::item_add(ItemType type, int animationIndex, const Element& item, int insertBeforeId,
                      types::destination::Type destination)
  {
    auto row = track_container_get(type);
    auto elements = row ? element_get(row->elements) : nullptr;
    if (!elements) return -1;

    auto id = item.id == -1 ? element_child_next_id_get(*elements, row->element) : item.id;
    auto element = child_id_get(*elements, row->element, id);
    if (!element)
    {
      element = &elements->children.emplace_back(element_make(row->element));
      element->id = id;
    }

    if (!item.name.empty()) element->name = item.name;
    element->spritesheetId = element_get(ElementType::SPRITESHEET, item.spritesheetId) ? item.spritesheetId : 0;
    element->isShowRect = item.isShowRect;

    auto add = [&](Element& animation)
    {
      if (animation_item_get(animation, type, id)) return;
      auto& tracks = child_ensure(animation, row->container);
      auto track = element_make(row->track);
      track.*(row->id) = id;
      auto before = std::ranges::find_if(tracks.children, [&](const Element& child)
                                         { return child.type == row->track && track_id_get(child) == insertBeforeId; });
      tracks.children.insert(insertBeforeId == -1 ? tracks.children.end() : before, track);
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
