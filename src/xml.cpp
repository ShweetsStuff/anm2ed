#include "xml.h"

#include "math.h"

namespace anm2ed::xml
{
  std::string document_to_string(tinyxml2::XMLDocument& self)
  {
    tinyxml2::XMLPrinter printer{};
    self.Print(&printer);
    return std::string(printer.CStr());
  }

  tinyxml2::XMLError query_string_attribute(tinyxml2::XMLElement* element, const char* attribute, std::string* out)
  {
    const char* temp = nullptr;
    auto result = element->QueryStringAttribute(attribute, &temp);
    if (result == tinyxml2::XML_SUCCESS && temp) *out = temp;
    return result;
  }

  tinyxml2::XMLError query_path_attribute(tinyxml2::XMLElement* element, const char* attribute,
                                          std::filesystem::path* out)
  {
    std::string temp{};
    auto result = query_string_attribute(element, attribute, &temp);
    if (result == tinyxml2::XML_SUCCESS) *out = temp;
    return result;
  }

  void query_color_attribute(tinyxml2::XMLElement* element, const char* attribute, float& out)
  {
    int value{};
    element->QueryIntAttribute(attribute, &value);
    out = math::uint8_to_float(value);
  }
}