#include "util.h"

#include <algorithm>
#include <chrono>

namespace anm2ed::util::time
{
  std::string get(const char* format)
  {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto localTime = *std::localtime(&time);
    std::ostringstream timeString;
    timeString << std::put_time(&localTime, format);
    return timeString.str();
  }
}

namespace anm2ed::util::string
{
  std::string to_lower(const std::string& string)
  {
    std::string transformed = string;
    std::ranges::transform(transformed, transformed.begin(), [](const unsigned char c) { return std::tolower(c); });
    return transformed;
  }

  std::string replace_backslash(const std::string& string)
  {
    std::string transformed = string;
    for (char& character : transformed)
      if (character == '\\') character = '/';
    return transformed;
  }

  bool to_bool(const std::string& string)
  {
    return to_lower(string) == "true";
  }
}
