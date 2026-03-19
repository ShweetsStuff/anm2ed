#include "anm2.hpp"

#include <ranges>

#include "map_.hpp"

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
    labels.emplace_back(localize.get(BASIC_NONE));
    for (auto& event : content.events | std::views::values)
      labels.emplace_back(event.name);
    return labels;
  }

  std::vector<int> Anm2::event_ids_get()
  {
    std::vector<int> ids{};
    ids.emplace_back(-1);
    for (auto& id : content.events | std::views::keys)
      ids.emplace_back(id);
    return ids;
  }

  std::set<int> Anm2::events_unused()
  {
    std::set<int> used{};

    for (auto& animation : animations.items)
      for (auto& frame : animation.triggers.frames)
        if (frame.eventID != -1) used.insert(frame.eventID);

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
