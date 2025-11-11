#include "event.h"

#include "xml_.h"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Event::Event(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_string_attribute(element, "Name", &name);
  }

  XMLElement* Event::to_element(XMLDocument& document, int id)
  {
    auto element = document.NewElement("Event");
    element->SetAttribute("Id", id);
    element->SetAttribute("Name", name.c_str());
    return element;
  }

  void Event::serialize(XMLDocument& document, XMLElement* parent, int id)
  {
    parent->InsertEndChild(to_element(document, id));
  }

  std::string Event::to_string(int id)
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, id));
    return xml::document_to_string(document);
  }
}