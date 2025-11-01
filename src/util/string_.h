#pragma once

#include <string>

namespace anm2ed::util::string
{
  std::string to_lower(const std::string&);
  std::string backslash_replace(const std::string&);
  std::string backslash_replace_to_lower(const std::string&);
  bool to_bool(const std::string&);
}
