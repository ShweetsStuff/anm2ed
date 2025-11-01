#include "time_.h"

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