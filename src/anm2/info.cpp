#include "info.h"

#include "xml_.h"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Info::Info(XMLElement* element)
  {
    if (!element) return;
    xml::query_string_attribute(element, "CreatedBy", &createdBy);
    xml::query_string_attribute(element, "CreatedOn", &createdOn);
    element->QueryIntAttribute("Fps", &fps);
    element->QueryIntAttribute("Version", &version);
  }

  XMLElement* Info::to_element(XMLDocument& document)
  {
    auto element = document.NewElement("Info");
    element->SetAttribute("CreatedBy", createdBy.c_str());
    element->SetAttribute("CreatedOn", createdOn.c_str());
    element->SetAttribute("Fps", fps);
    element->SetAttribute("Version", version);
    return element;
  }

  void Info::serialize(XMLDocument& document, XMLElement* parent)
  {
    parent->InsertEndChild(to_element(document));
  }

  std::string Info::to_string()
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document));
    return xml::document_to_string(document);
  }
}