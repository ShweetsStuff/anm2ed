#pragma once

#include <filesystem>
#include <string>

#include <tinyxml2/tinyxml2.h>

namespace anm2ed::util::xml
{
  std::string document_to_string(tinyxml2::XMLDocument&);

  bool query_string_attribute(const tinyxml2::XMLElement*, const char*, std::string&);
  bool query_path_attribute(const tinyxml2::XMLElement*, const char*, std::filesystem::path&);
  bool query_int_attribute(const tinyxml2::XMLElement*, const char*, int&);
  bool query_float_attribute(const tinyxml2::XMLElement*, const char*, float&);
  bool query_bool_attribute(const tinyxml2::XMLElement*, const char*, bool&);
  bool query_color_attribute(const tinyxml2::XMLElement*, const char*, float&);

  void set_string_attribute(tinyxml2::XMLElement*, const char*, const std::string&);
  void set_path_attribute(tinyxml2::XMLElement*, const char*, const std::filesystem::path&);
  void set_int_attribute(tinyxml2::XMLElement*, const char*, int);
  void set_float_attribute(tinyxml2::XMLElement*, const char*, float);
  void set_bool_attribute(tinyxml2::XMLElement*, const char*, bool);
  void set_color_attribute(tinyxml2::XMLElement*, const char*, float);

  tinyxml2::XMLError query_string_attribute(tinyxml2::XMLElement*, const char*, std::string*);
  tinyxml2::XMLError query_path_attribute(tinyxml2::XMLElement*, const char*, std::filesystem::path*);
  void query_color_attribute(tinyxml2::XMLElement*, const char*, float&);
}
