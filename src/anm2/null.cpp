#include "null.h"

#include "xml_.h"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Null::Null(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_string_attribute(element, "Name", &name);
    element->QueryBoolAttribute("ShowRect", &isShowRect);
  }

  XMLElement* Null::to_element(XMLDocument& document, int id)
  {
    auto element = document.NewElement("Null");
    element->SetAttribute("Id", id);
    element->SetAttribute("Name", name.c_str());
    if (isShowRect) element->SetAttribute("ShowRect", isShowRect);
    return element;
  }

  void Null::serialize(XMLDocument& document, XMLElement* parent, int id)
  {
    parent->InsertEndChild(to_element(document, id));
  }

  std::string Null::to_string(int id)
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, id));
    return xml::document_to_string(document);
  }
}