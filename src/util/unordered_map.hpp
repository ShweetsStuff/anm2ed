#pragma once

#include <unordered_map>

namespace anm2ed::util::unordered_map
{
  template <typename T0, typename T1> T1* find(std::unordered_map<T0, T1>& map, T0 index)
  {
    return map.contains(index) ? &map[index] : nullptr;
  }
}
