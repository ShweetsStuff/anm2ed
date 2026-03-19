#include "anm2.hpp"

#include "map_.hpp"
#include "types.hpp"
#include "unordered_map_.hpp"

using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::anm2
{
  Item* Anm2::item_get(int animationIndex, Type type, int id)
  {
    if (Animation* animation = animation_get(animationIndex))
    {
      switch (type)
      {
        case ROOT:
          return &animation->rootAnimation;
        case LAYER:
          return unordered_map::find(animation->layerAnimations, id);
        case NULL_:
          return map::find(animation->nullAnimations, id);
        case TRIGGER:
          return &animation->triggers;
        default:
          return nullptr;
      }
    }
    return nullptr;
  }

  Reference Anm2::layer_animation_add(Reference reference, int insertBeforeID, std::string name, int spritesheetID,
                                      destination::Type destination)
  {
    auto id = reference.itemID == -1 ? map::next_id_get(content.layers) : reference.itemID;
    auto& layer = content.layers[id];

    layer.name = !name.empty() ? name : layer.name;
    layer.spritesheetID = content.spritesheets.contains(spritesheetID) ? spritesheetID : 0;

    auto add = [&](Animation* animation, int id, bool insertBeforeReference)
    {
      animation->layerAnimations[id] = Item();

      if (insertBeforeReference && insertBeforeID != -1)
      {
        auto it = std::find(animation->layerOrder.begin(), animation->layerOrder.end(), insertBeforeID);
        if (it != animation->layerOrder.end())
        {
          animation->layerOrder.insert(it, id);
          return;
        }
      }

      animation->layerOrder.push_back(id);
    };

    if (destination == destination::ALL)
    {
      for (size_t index = 0; index < animations.items.size(); ++index)
      {
        auto& animation = animations.items[index];
        if (!animation.layerAnimations.contains(id)) add(&animation, id, true);
      }
    }
    else if (destination == destination::THIS)
    {
      if (auto animation = animation_get(reference.animationIndex))
        if (!animation->layerAnimations.contains(id)) add(animation, id, true);
    }

    return {reference.animationIndex, LAYER, id};
  }

  Reference Anm2::null_animation_add(Reference reference, std::string name, bool isShowRect, destination::Type destination)
  {
    auto id = reference.itemID == -1 ? map::next_id_get(content.nulls) : reference.itemID;
    auto& null = content.nulls[id];

    null.name = !name.empty() ? name : null.name;
    null.isShowRect = isShowRect;

    auto add = [&](Animation* animation, int id) { animation->nullAnimations[id] = Item(); };

    if (destination == destination::ALL)
    {
      for (auto& animation : animations.items)
        if (!animation.nullAnimations.contains(id)) add(&animation, id);
    }
    else if (destination == destination::THIS)
    {
      if (auto animation = animation_get(reference.animationIndex))
        if (!animation->nullAnimations.contains(id)) add(animation, id);
    }

    return {reference.animationIndex, NULL_, id};
  }
}
