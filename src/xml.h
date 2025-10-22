#pragma once

#include <filesystem>
#include <string>

#include <tinyxml2/tinyxml2.h>

namespace anm2ed::xml
{
  std::string document_to_string(tinyxml2::XMLDocument& self);
  tinyxml2::XMLError query_string_attribute(tinyxml2::XMLElement* element, const char* attribute, std::string* out);
  tinyxml2::XMLError query_path_attribute(tinyxml2::XMLElement* element, const char* attribute,
                                          std::filesystem::path* out);
  void query_color_attribute(tinyxml2::XMLElement* element, const char* attribute, float& out);
}