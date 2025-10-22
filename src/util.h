#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace anm2ed::util::time
{
  std::string get(const char* format);
}

namespace anm2ed::util::string
{
  std::string to_lower(const std::string& string);
  std::string replace_backslash(const std::string& string);
  bool to_bool(const std::string& string);
}

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

namespace anm2ed::util::unordered_map
{
  template <typename T0, typename T1> T1* find(std::unordered_map<T0, T1>& map, T0 index)
  {
    return map.contains(index) ? &map[index] : nullptr;
  }
}

namespace anm2ed::util::vector
{
  template <typename T> T* find(std::vector<T>& v, int index)
  {
    return index >= 0 && index < (int)v.size() ? &v[index] : nullptr;
  }
}