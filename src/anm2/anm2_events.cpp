#include "anm2.h"

#include <ranges>

#include "map_.h"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  void Anm2::event_add(int& id)
  {
    id = map::next_id_get(content.events);
    content.events[id] = Event();
  }

  std::vector<std::string> Anm2::event_labels_get()
  {
    std::vector<std::string> labels{};
    labels.emplace_back("None");
    for (auto& event : content.events | std::views::values)
      labels.emplace_back(event.name);
    return labels;
  }

  std::set<int> Anm2::events_unused(Reference reference)
  {
    std::set<int> used{};

    if (auto animation = animation_get(reference); animation)
      for (auto& frame : animation->triggers.frames)
        used.insert(frame.eventID);
    else
      for (auto& animation : animations.items)
        for (auto& frame : animation.triggers.frames)
          used.insert(frame.eventID);

    std::set<int> unused{};
    for (auto& id : content.events | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  bool Anm2::events_deserialize(const std::string& string, merge::Type type, std::string* errorString)
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
        auto event = Event(element, id);
        if (type == merge::APPEND) id = map::next_id_get(content.events);
        content.events[id] = event;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }
}
