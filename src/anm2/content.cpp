#include "content.h"

using namespace tinyxml2;

namespace anm2ed::anm2
{
  Content::Content(XMLElement* element)
  {
    int id{};

    if (auto spritesheetsElement = element->FirstChildElement("Spritesheets"))
      for (auto child = spritesheetsElement->FirstChildElement("Spritesheet"); child;
           child = child->NextSiblingElement("Spritesheet"))
        spritesheets[id] = Spritesheet(child, id);

    if (auto layersElement = element->FirstChildElement("Layers"))
      for (auto child = layersElement->FirstChildElement("Layer"); child; child = child->NextSiblingElement("Layer"))
        layers[id] = Layer(child, id);

    if (auto nullsElement = element->FirstChildElement("Nulls"))
      for (auto child = nullsElement->FirstChildElement("Null"); child; child = child->NextSiblingElement("Null"))
        nulls[id] = Null(child, id);

    if (auto eventsElement = element->FirstChildElement("Events"))
      for (auto child = eventsElement->FirstChildElement("Event"); child; child = child->NextSiblingElement("Event"))
        events[id] = Event(child, id);

    if (auto eventsElement = element->FirstChildElement("Sounds"))
      for (auto child = eventsElement->FirstChildElement("Sound"); child; child = child->NextSiblingElement("Sound"))
        sounds[id] = Sound(child, id);
  }

  void Content::serialize(XMLDocument& document, XMLElement* parent)
  {
    auto element = document.NewElement("Content");

    auto spritesheetsElement = document.NewElement("Spritesheets");
    for (auto& [id, spritesheet] : spritesheets)
      spritesheet.serialize(document, spritesheetsElement, id);
    element->InsertEndChild(spritesheetsElement);

    auto layersElement = document.NewElement("Layers");
    for (auto& [id, layer] : layers)
      layer.serialize(document, layersElement, id);
    element->InsertEndChild(layersElement);

    auto nullsElement = document.NewElement("Nulls");
    for (auto& [id, null] : nulls)
      null.serialize(document, nullsElement, id);
    element->InsertEndChild(nullsElement);

    auto eventsElement = document.NewElement("Events");
    for (auto& [id, event] : events)
      event.serialize(document, eventsElement, id);
    element->InsertEndChild(eventsElement);

    if (!sounds.empty())
    {
      auto soundsElement = document.NewElement("Sounds");
      for (auto& [id, sound] : sounds)
        sound.serialize(document, soundsElement, id);
      element->InsertEndChild(soundsElement);
    }

    parent->InsertEndChild(element);
  }

}
