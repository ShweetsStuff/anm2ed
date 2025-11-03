#include "anm2.h"

#include <ranges>

#include "map_.h"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  void Anm2::layer_add(int& id)
  {
    id = map::next_id_get(content.layers);
    content.layers[id] = Layer();
  }

  std::set<int> Anm2::layers_unused(Reference reference)
  {
    std::set<int> used{};
    std::set<int> unused{};

    if (auto animation = animation_get(reference); animation)
      for (auto& id : animation->layerAnimations | std::views::keys)
        used.insert(id);
    else
      for (auto& animation : animations.items)
        for (auto& id : animation.layerAnimations | std::views::keys)
          used.insert(id);

    for (auto& id : content.layers | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  bool Anm2::layers_deserialize(const std::string& string, merge::Type type, std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Layer"))
      {
        if (errorString) *errorString = "No valid layer(s).";
        return false;
      }

      for (auto element = document.FirstChildElement("Layer"); element; element = element->NextSiblingElement("Layer"))
      {
        auto layer = Layer(element, id);
        if (type == merge::APPEND) id = map::next_id_get(content.layers);
        content.layers[id] = layer;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }
}
