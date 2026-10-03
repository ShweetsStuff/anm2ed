#pragma once

#include <string>

#include <tinyxml2/tinyxml2.h>

namespace anm2ed::util::xml
{
  std::string document_to_string(tinyxml2::XMLDocument&);
}
