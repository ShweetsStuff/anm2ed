#include "xml.hpp"

using namespace tinyxml2;

namespace anm2ed::util::xml
{
  std::string document_to_string(XMLDocument& self)
  {
    XMLPrinter printer{};
    self.Print(&printer);
    return std::string(printer.CStr());
  }
}
