#include "anm2.h"

#include "map_.h"
#include "types.h"
#include "unordered_map_.h"

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

  Reference Anm2::layer_animation_add(Reference reference, std::string name, int spritesheetID,
                                      destination::Type locale)
  {
    auto id = reference.itemID == -1 ? map::next_id_get(content.layers) : reference.itemID;
    auto& layer = content.layers[id];

    layer.name = !name.empty() ? name : layer.name;
    layer.spritesheetID = content.spritesheets.contains(spritesheetID) ? spritesheetID : 0;

    auto add = [&](Animation* animation, int id)
    {
      animation->layerAnimations[id] = Item();
      animation->layerOrder.push_back(id);
    };

    if (locale == destination::ALL)
    {
      for (auto& animation : animations.items)
        if (!animation.layerAnimations.contains(id)) add(&animation, id);
    }
    else if (locale == destination::THIS)
    {
      if (auto animation = animation_get(reference.animationIndex))
        if (!animation->layerAnimations.contains(id)) add(animation, id);
    }

    return {reference.animationIndex, LAYER, id};
  }

  Reference Anm2::null_animation_add(Reference reference, std::string name, destination::Type locale)
  {
    auto id = reference.itemID == -1 ? map::next_id_get(content.nulls) : reference.itemID;
    auto& null = content.nulls[id];

    null.name = !name.empty() ? name : null.name;

    auto add = [&](Animation* animation, int id) { animation->nullAnimations[id] = Item(); };

    if (locale == destination::ALL)
    {
      for (auto& animation : animations.items)
        if (!animation.nullAnimations.contains(id)) add(&animation, id);
    }
    else if (locale == destination::THIS)
    {
      if (auto animation = animation_get(reference.animationIndex))
        if (!animation->nullAnimations.contains(id)) add(animation, id);
    }

    return {reference.animationIndex, LAYER, id};
  }
}