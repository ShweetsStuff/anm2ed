#pragma once

#include <atomic>
#include <cstdint>

namespace anm2ed::util
{
  // Process-unique, never reused; 0 means "none".
  inline std::uint64_t uid_next()
  {
    static std::atomic<std::uint64_t> next{1};
    return next++;
  }
}
