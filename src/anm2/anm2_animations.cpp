#include "anm2.h"

#include "vector_.h"

using namespace anm2ed::util;
using namespace anm2ed::types;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Animation* Anm2::animation_get(Reference reference)
  {
    return vector::find(animations.items, reference.animationIndex);
  }

  std::vector<std::string> Anm2::animation_labels_get()
  {
    std::vector<std::string> labels{};
    labels.emplace_back("None");
    for (auto& animation : animations.items)
      labels.emplace_back(animation.name);
    return labels;
  }

  bool Anm2::animations_deserialize(const std::string& string, int start, std::set<int>& indices,
                                    std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      if (!document.FirstChildElement("Animation"))
      {
        if (errorString) *errorString = "No valid animation(s).";
        return false;
      }

      int count{};
      for (auto element = document.FirstChildElement("Animation"); element;
           element = element->NextSiblingElement("Animation"))
      {
        auto index = start + count;
        animations.items.insert(animations.items.begin() + start + count, Animation(element));
        indices.insert(index);
        count++;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  int Anm2::animations_merge(int target, std::set<int>& sources, merge::Type type, bool isDeleteAfter)
  {
    auto& items = animations.items;
    auto& animation = animations.items.at(target);

    if (!animation.name.ends_with(MERGED_STRING)) animation.name = animation.name + " " + MERGED_STRING;

    auto merge_item = [&](Item& destination, Item& source)
    {
      switch (type)
      {
        case merge::APPEND:
          destination.frames.insert(destination.frames.end(), source.frames.begin(), source.frames.end());
          break;
        case merge::PREPEND:
          destination.frames.insert(destination.frames.begin(), source.frames.begin(), source.frames.end());
          break;
        case merge::REPLACE:
          if (destination.frames.size() < source.frames.size()) destination.frames.resize(source.frames.size());
          for (int i = 0; i < (int)source.frames.size(); i++)
            destination.frames[i] = source.frames[i];
          break;
        case merge::IGNORE:
        default:
          break;
      }
    };

    for (auto& i : sources)
    {
      if (i == target) continue;
      if (i < 0 || i >= (int)items.size()) continue;

      auto& source = items.at(i);

      merge_item(animation.rootAnimation, source.rootAnimation);

      for (auto& [id, layerAnimation] : source.layerAnimations)
      {
        if (!animation.layerAnimations.contains(id))
        {
          animation.layerAnimations[id] = layerAnimation;
          animation.layerOrder.emplace_back(id);
        }
        merge_item(animation.layerAnimations[id], layerAnimation);
      }

      for (auto& [id, nullAnimation] : source.nullAnimations)
      {
        if (!animation.nullAnimations.contains(id)) animation.nullAnimations[id] = nullAnimation;
        merge_item(animation.nullAnimations[id], nullAnimation);
      }

      merge_item(animation.triggers, source.triggers);
    }

    if (isDeleteAfter)
    {
      for (auto& source : std::ranges::reverse_view(sources))
      {
        if (source == target) continue;
        items.erase(items.begin() + source);
      }
    }

    int finalIndex = target;

    if (isDeleteAfter)
    {
      int numDeletedBefore = 0;
      for (auto& idx : sources)
      {
        if (idx == target) continue;
        if (idx >= 0 && idx < target) ++numDeletedBefore;
      }
      finalIndex -= numDeletedBefore;
    }

    return finalIndex;
  }

}