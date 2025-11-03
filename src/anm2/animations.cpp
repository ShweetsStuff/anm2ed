#include "animations.h"

#include "xml_.h"

using namespace tinyxml2;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::anm2
{
  Animations::Animations(XMLElement* element)
  {
    xml::query_string_attribute(element, "DefaultAnimation", &defaultAnimation);

    for (auto child = element->FirstChildElement("Animation"); child; child = child->NextSiblingElement("Animation"))
      items.push_back(Animation(child));
  }

  XMLElement* Animations::to_element(XMLDocument& document)
  {
    auto element = document.NewElement("Animations");
    element->SetAttribute("DefaultAnimation", defaultAnimation.c_str());
    for (auto& animation : items)
      animation.serialize(document, element);
    return element;
  }

  void Animations::serialize(XMLDocument& document, XMLElement* parent)
  {
    parent->InsertEndChild(to_element(document));
  }

  int Animations::length()
  {
    int length{};

    for (auto& animation : items)
      if (int animationLength = animation.length(); animationLength > length) length = animationLength;

    return length;
  }

}