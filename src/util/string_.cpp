#include "string_.h"

#include <algorithm>

namespace anm2ed::util::string
{
  std::string to_lower(const std::string& string)
  {
    std::string transformed = string;
    std::ranges::transform(transformed, transformed.begin(), [](const unsigned char c) { return std::tolower(c); });
    return transformed;
  }

  std::string backslash_replace(const std::string& string)
  {
    std::string transformed = string;
    for (char& character : transformed)
      if (character == '\\') character = '/';
    return transformed;
  }

  std::string backslash_replace_to_lower(const std::string& string)
  {
    std::string transformed = string;
    transformed = backslash_replace(transformed);
    return to_lower(transformed);
  }

  std::string quote(const std::string& string)
  {
    return "\"" + string + "\"";
  }

  bool to_bool(const std::string& string)
  {
    return to_lower(string) == "true";
  }
}
