#include "anm2.h"

#include "map_.h"
#include "types.h"
#include "unordered_map_.h"

using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::anm2
{
  Item* Anm2::item_get(Reference reference)
  {
    if (Animation* animation = animation_get(reference))
    {
      switch (reference.itemType)
      {
        case ROOT:
          return &animation->rootAnimation;
        case LAYER:
          return unordered_map::find(animation->layerAnimations, reference.itemID);
        case NULL_:
          return map::find(animation->nullAnimations, reference.itemID);
        case TRIGGER:
          return &animation->triggers;
        default:
          return nullptr;
      }
    }
    return nullptr;
  }

  Reference Anm2::layer_animation_add(Reference reference, std::string name, int spritesheetID, locale::Type locale)
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

    if (locale == locale::GLOBAL)
    {
      for (auto& animation : animations.items)
        if (!animation.layerAnimations.contains(id)) add(&animation, id);
    }
    else if (locale == locale::LOCAL)
    {
      if (auto animation = animation_get(reference))
        if (!animation->layerAnimations.contains(id)) add(animation, id);
    }

    return {reference.animationIndex, LAYER, id};
  }

  Reference Anm2::null_animation_add(Reference reference, std::string name, locale::Type locale)
  {
    auto id = reference.itemID == -1 ? map::next_id_get(content.nulls) : reference.itemID;
    auto& null = content.nulls[id];

    null.name = !name.empty() ? name : null.name;

    auto add = [&](Animation* animation, int id) { animation->nullAnimations[id] = Item(); };

    if (locale == locale::GLOBAL)
    {
      for (auto& animation : animations.items)
        if (!animation.nullAnimations.contains(id)) add(&animation, id);
    }
    else if (locale == locale::LOCAL)
    {
      if (auto animation = animation_get(reference))
        if (!animation->nullAnimations.contains(id)) add(animation, id);
    }

    return {reference.animationIndex, LAYER, id};
  }
}