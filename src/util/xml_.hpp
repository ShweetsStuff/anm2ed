#pragma once

#include <filesystem>
#include <string>

#include <tinyxml2/tinyxml2.h>

namespace anm2ed::util::xml
{
  std::string document_to_string(tinyxml2::XMLDocument&);
  tinyxml2::XMLError query_string_attribute(tinyxml2::XMLElement*, const char*, std::string*);
  tinyxml2::XMLError query_path_attribute(tinyxml2::XMLElement*, const char*, std::filesystem::path*);
  void query_color_attribute(tinyxml2::XMLElement*, const char*, float&);
}
