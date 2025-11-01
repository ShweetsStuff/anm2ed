#include "content.h"

#include <ranges>

#include "filesystem_.h"
#include "map_.h"

using namespace anm2ed::types;
using namespace anm2ed::util;
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

    parent->InsertEndChild(element);
  }

  std::set<int> Content::spritesheets_unused()
  {
    std::set<int> used;
    for (auto& layer : layers | std::views::values)
      if (layer.spritesheetID != -1) used.insert(layer.spritesheetID);

    std::set<int> unused;
    for (auto& id : spritesheets | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  void Content::layer_add(int& id)
  {
    id = map::next_id_get(layers);
    layers[id] = Layer();
  }

  void Content::null_add(int& id)
  {
    id = map::next_id_get(nulls);
    nulls[id] = Null();
  }

  void Content::event_add(int& id)
  {
    id = map::next_id_get(events);
    events[id] = Event();
  }

  bool Content::spritesheets_deserialize(const std::string& string, const std::string& directory, merge::Type type,
                                         std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Spritesheet"))
      {
        if (errorString) *errorString = "No valid spritesheet(s).";
        return false;
      }

      filesystem::WorkingDirectory workingDirectory(directory);

      for (auto element = document.FirstChildElement("Spritesheet"); element;
           element = element->NextSiblingElement("Spritesheet"))
      {
        auto spritesheet = Spritesheet(element, id);

        if (type == merge::APPEND) id = map::next_id_get(spritesheets);

        spritesheets[id] = std::move(spritesheet);
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  bool Content::layers_deserialize(const std::string& string, merge::Type type, std::string* errorString)
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

        if (type == merge::APPEND) id = map::next_id_get(layers);

        layers[id] = layer;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  bool Content::nulls_deserialize(const std::string& string, merge::Type type, std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Null"))
      {
        if (errorString) *errorString = "No valid null(s).";
        return false;
      }

      for (auto element = document.FirstChildElement("Null"); element; element = element->NextSiblingElement("Null"))
      {
        auto layer = Null(element, id);

        if (type == merge::APPEND) id = map::next_id_get(nulls);

        nulls[id] = layer;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  bool Content::events_deserialize(const std::string& string, merge::Type type, std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Event"))
      {
        if (errorString) *errorString = "No valid event(s).";
        return false;
      }

      for (auto element = document.FirstChildElement("Event"); element; element = element->NextSiblingElement("Event"))
      {
        auto layer = Event(element, id);

        if (type == merge::APPEND) id = map::next_id_get(events);

        events[id] = layer;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  bool Content::sounds_deserialize(const std::string& string, const std::string& directory, merge::Type type,
                                   std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Sound"))
      {
        if (errorString) *errorString = "No valid sound(s).";
        return false;
      }

      filesystem::WorkingDirectory workingDirectory(directory);

      for (auto element = document.FirstChildElement("Sound"); element; element = element->NextSiblingElement("Sound"))
      {
        auto sound = Sound(element, id);

        if (type == merge::APPEND) id = map::next_id_get(sounds);

        sounds[id] = std::move(sound);
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

}