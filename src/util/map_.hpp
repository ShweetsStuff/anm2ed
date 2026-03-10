#pragma once

#include <map>

namespace anm2ed::util::map
{
  template <typename T> int next_id_get(std::map<int, T>& map)
  {
    int id = 0;

    for (auto& [key, value] : map)
    {
      if (key != id) break;
      ++id;
    }

    return id;
  }

  template <typename T0, typename T1> T1* find(std::map<T0, T1>& map, T0 index)
  {
    return map.contains(index) ? &map[index] : nullptr;
  }
}
