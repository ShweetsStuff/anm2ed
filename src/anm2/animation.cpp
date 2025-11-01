#include "animation.h"

#include "map_.h"
#include "math_.h"
#include "unordered_map_.h"
#include "xml_.h"
#include <ranges>

using namespace anm2ed::util;
using namespace glm;

using namespace tinyxml2;

namespace anm2ed::anm2
{
  Animation::Animation(XMLElement* element)
  {
    int id{};

    xml::query_string_attribute(element, "Name", &name);
    element->QueryIntAttribute("FrameNum", &frameNum);
    element->QueryBoolAttribute("Loop", &isLoop);

    if (auto rootAnimationElement = element->FirstChildElement("RootAnimation"))
      rootAnimation = Item(rootAnimationElement, ROOT);

    if (auto layerAnimationsElement = element->FirstChildElement("LayerAnimations"))
    {
      for (auto child = layerAnimationsElement->FirstChildElement("LayerAnimation"); child;
           child = child->NextSiblingElement("LayerAnimation"))
      {
        layerAnimations[id] = Item(child, LAYER, &id);
        layerOrder.push_back(id);
      }
    }

    if (auto nullAnimationsElement = element->FirstChildElement("NullAnimations"))
      for (auto child = nullAnimationsElement->FirstChildElement("NullAnimation"); child;
           child = child->NextSiblingElement("NullAnimation"))
        nullAnimations[id] = Item(child, NULL_, &id);

    if (auto triggersElement = element->FirstChildElement("Triggers")) triggers = Item(triggersElement, TRIGGER);
  }

  Item* Animation::item_get(Type type, int id)
  {
    switch (type)
    {
      case ROOT:
        return &rootAnimation;
      case LAYER:
        return unordered_map::find(layerAnimations, id);
      case NULL_:
        return map::find(nullAnimations, id);
      case TRIGGER:
        return &triggers;
      default:
        return nullptr;
    }
    return nullptr;
  }

  void Animation::item_remove(Type type, int id)
  {
    switch (type)
    {
      case LAYER:
        layerAnimations.erase(id);
        for (auto [i, value] : std::views::enumerate(layerOrder))
          if (value == id) layerOrder.erase(layerOrder.begin() + i);
        break;
      case NULL_:
        nullAnimations.erase(id);
        break;
      case ROOT:
      case TRIGGER:
      default:
        break;
    }
  }

  void Animation::serialize(XMLDocument& document, XMLElement* parent)
  {
    auto element = document.NewElement("Animation");
    element->SetAttribute("Name", name.c_str());
    element->SetAttribute("FrameNum", frameNum);
    element->SetAttribute("Loop", isLoop);

    rootAnimation.serialize(document, element, ROOT);

    auto layerAnimationsElement = document.NewElement("LayerAnimations");
    for (auto& i : layerOrder)
    {
      Item& layerAnimation = layerAnimations.at(i);
      layerAnimation.serialize(document, layerAnimationsElement, LAYER, i);
    }
    element->InsertEndChild(layerAnimationsElement);

    auto nullAnimationsElement = document.NewElement("NullAnimations");
    for (auto& [id, nullAnimation] : nullAnimations)
      nullAnimation.serialize(document, nullAnimationsElement, NULL_, id);
    element->InsertEndChild(nullAnimationsElement);

    triggers.serialize(document, element, TRIGGER);

    parent->InsertEndChild(element);
  }

  int Animation::length()
  {
    int length{};

    if (int rootAnimationLength = rootAnimation.length(ROOT); rootAnimationLength > length)
      length = rootAnimationLength;

    for (auto& layerAnimation : layerAnimations | std::views::values)
      if (int layerAnimationLength = layerAnimation.length(LAYER); layerAnimationLength > length)
        length = layerAnimationLength;

    for (auto& nullAnimation : nullAnimations | std::views::values)
      if (int nullAnimationLength = nullAnimation.length(NULL_); nullAnimationLength > length)
        length = nullAnimationLength;

    if (int triggersLength = triggers.length(TRIGGER); triggersLength > length) length = triggersLength;

    return length;
  }

  std::string Animation::to_string()
  {
    XMLDocument document{};

    auto* element = document.NewElement("Animation");
    document.InsertFirstChild(element);

    element->SetAttribute("Name", name.c_str());
    element->SetAttribute("FrameNum", frameNum);
    element->SetAttribute("Loop", isLoop);

    rootAnimation.serialize(document, element, ROOT);

    auto layerAnimationsElement = document.NewElement("LayerAnimations");
    for (auto& i : layerOrder)
    {
      Item& layerAnimation = layerAnimations.at(i);
      layerAnimation.serialize(document, layerAnimationsElement, LAYER, i);
    }
    element->InsertEndChild(layerAnimationsElement);

    auto nullAnimationsElement = document.NewElement("NullAnimations");
    for (auto& [id, nullAnimation] : nullAnimations)
      nullAnimation.serialize(document, nullAnimationsElement, NULL_, id);
    element->InsertEndChild(nullAnimationsElement);

    triggers.serialize(document, element, TRIGGER);

    XMLPrinter printer;
    document.Print(&printer);
    return std::string(printer.CStr());
  }

  vec4 Animation::rect(bool isRootTransform)
  {
    float minX = std::numeric_limits<float>::infinity();
    float minY = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    float maxY = -std::numeric_limits<float>::infinity();
    bool any = false;

    constexpr ivec2 CORNERS[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    for (float t = 0.0f; t < (float)frameNum; t += 1.0f)
    {
      mat4 transform(1.0f);

      if (isRootTransform)
      {
        auto root = rootAnimation.frame_generate(t, anm2::ROOT);
        transform *= math::quad_model_parent_get(root.position, {}, math::percent_to_unit(root.scale), root.rotation);
      }

      for (auto& [id, layerAnimation] : layerAnimations)
      {
        auto frame = layerAnimation.frame_generate(t, anm2::LAYER);

        if (frame.size == vec2() || !frame.isVisible) continue;

        auto layerTransform = transform * math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                               math::percent_to_unit(frame.scale), frame.rotation);
        for (auto& corner : CORNERS)
        {
          vec4 world = layerTransform * vec4(corner, 0.0f, 1.0f);
          minX = std::min(minX, world.x);
          minY = std::min(minY, world.y);
          maxX = std::max(maxX, world.x);
          maxY = std::max(maxY, world.y);
          any = true;
        }
      }
    }

    if (!any) return vec4(-1.0f);
    return {minX, minY, maxX - minX, maxY - minY};
  }

}