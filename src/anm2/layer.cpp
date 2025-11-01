#include "layer.h"

#include "xml_.h"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Layer::Layer(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_string_attribute(element, "Name", &name);
    element->QueryIntAttribute("SpritesheetId", &spritesheetID);
  }

  XMLElement* Layer::to_element(XMLDocument& document, int id)
  {
    auto element = document.NewElement("Layer");
    element->SetAttribute("Id", id);
    element->SetAttribute("Name", name.c_str());
    element->SetAttribute("SpritesheetId", spritesheetID);
    return element;
  }

  void Layer::serialize(XMLDocument& document, XMLElement* parent, int id)
  {
    parent->InsertEndChild(to_element(document, id));
  }

  std::string Layer::to_string(int id)
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, id));
    return xml::document_to_string(document);
  }

}