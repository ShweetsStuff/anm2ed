#include "xml_.h"

#include "filesystem_.h"
#include "math_.h"

using namespace tinyxml2;
namespace filesystem = anm2ed::util::filesystem;

namespace anm2ed::util::xml
{
  std::string document_to_string(XMLDocument& self)
  {
    XMLPrinter printer{};
    self.Print(&printer);
    return std::string(printer.CStr());
  }

  XMLError query_string_attribute(XMLElement* element, const char* attribute, std::string* out)
  {
    const char* temp = nullptr;
    auto result = element->QueryStringAttribute(attribute, &temp);
    if (result == XML_SUCCESS && temp) *out = temp;
    return result;
  }

  XMLError query_path_attribute(XMLElement* element, const char* attribute, std::filesystem::path* out)
  {
    std::string temp{};
    auto result = query_string_attribute(element, attribute, &temp);
    if (result == XML_SUCCESS) *out = filesystem::path_from_utf8(temp);
    return result;
  }

  void query_color_attribute(XMLElement* element, const char* attribute, float& out)
  {
    int value{};
    element->QueryIntAttribute(attribute, &value);
    out = math::uint8_to_float(value);
  }
}
