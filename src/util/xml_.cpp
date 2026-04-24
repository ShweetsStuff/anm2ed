#include "xml_.hpp"

#include "math_.hpp"
#include "path_.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::util::xml
{
  std::string document_to_string(XMLDocument& self)
  {
    XMLPrinter printer{};
    self.Print(&printer);
    return std::string(printer.CStr());
  }

  bool query_string_attribute(const XMLElement* element, const char* attribute, std::string& out)
  {
    if (!element || !attribute) return false;

    const char* value = nullptr;
    if (element->QueryStringAttribute(attribute, &value) != XML_SUCCESS || !value) return false;
    out = value;
    return true;
  }

  bool query_path_attribute(const XMLElement* element, const char* attribute, std::filesystem::path& out)
  {
    std::string temp{};
    if (!query_string_attribute(element, attribute, temp)) return false;
    out = path::from_utf8(temp);
    return true;
  }

  bool query_int_attribute(const XMLElement* element, const char* attribute, int& out)
  {
    return element && attribute && element->QueryIntAttribute(attribute, &out) == XML_SUCCESS;
  }

  bool query_float_attribute(const XMLElement* element, const char* attribute, float& out)
  {
    return element && attribute && element->QueryFloatAttribute(attribute, &out) == XML_SUCCESS;
  }

  bool query_bool_attribute(const XMLElement* element, const char* attribute, bool& out)
  {
    return element && attribute && element->QueryBoolAttribute(attribute, &out) == XML_SUCCESS;
  }

  bool query_color_attribute(const XMLElement* element, const char* attribute, float& out)
  {
    int value{};
    if (!query_int_attribute(element, attribute, value)) return false;
    out = math::uint8_to_float(value);
    return true;
  }

  void set_string_attribute(XMLElement* element, const char* attribute, const std::string& value)
  {
    if (element && attribute && !value.empty()) element->SetAttribute(attribute, value.c_str());
  }

  void set_path_attribute(XMLElement* element, const char* attribute, const std::filesystem::path& value)
  {
    if (!element || !attribute || value.empty()) return;
    auto utf8 = path::to_utf8(value);
    element->SetAttribute(attribute, utf8.c_str());
  }

  void set_int_attribute(XMLElement* element, const char* attribute, int value)
  {
    if (element && attribute) element->SetAttribute(attribute, value);
  }

  void set_float_attribute(XMLElement* element, const char* attribute, float value)
  {
    if (element && attribute) element->SetAttribute(attribute, value);
  }

  void set_bool_attribute(XMLElement* element, const char* attribute, bool value)
  {
    if (element && attribute) element->SetAttribute(attribute, value);
  }

  void set_color_attribute(XMLElement* element, const char* attribute, float value)
  {
    if (element && attribute) element->SetAttribute(attribute, math::float_to_uint8(value));
  }

  XMLError query_string_attribute(XMLElement* element, const char* attribute, std::string* out)
  {
    if (!element || !attribute || !out) return XML_NO_ATTRIBUTE;

    const char* value = nullptr;
    auto result = element->QueryStringAttribute(attribute, &value);
    if (result == XML_SUCCESS && value) *out = value;
    return result;
  }

  XMLError query_path_attribute(XMLElement* element, const char* attribute, std::filesystem::path* out)
  {
    if (!out) return XML_NO_ATTRIBUTE;

    std::string temp{};
    auto result = query_string_attribute(element, attribute, &temp);
    if (result == XML_SUCCESS) *out = path::from_utf8(temp);
    return result;
  }

  void query_color_attribute(XMLElement* element, const char* attribute, float& out)
  {
    query_color_attribute(static_cast<const XMLElement*>(element), attribute, out);
  }
}
