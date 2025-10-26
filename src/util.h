#pragma once

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace anm2ed::util::time
{
  std::string get(const char*);
}

namespace anm2ed::util::string
{
  std::string to_lower(const std::string&);
  std::string replace_backslash(const std::string&);
  bool to_bool(const std::string&);
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

  template <typename T> std::set<int> move_indices(std::vector<T>& v, std::vector<int>& indices, int index)
  {
    if (indices.empty()) return {};

    std::vector<int> sorted = indices;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    // Determine if we are dragging items from below the target (insert before) or above (insert after)
    bool insertAfter = !sorted.empty() && sorted.front() <= index;

    std::vector<T> moveItems;
    moveItems.reserve(sorted.size());
    for (int i : sorted)
      moveItems.push_back(std::move(v[i]));

    for (auto i : sorted | std::views::reverse)
      v.erase(v.begin() + i);

    int originalSize = (int)v.size() + (int)sorted.size();
    int reference = insertAfter ? index + 1 : index;
    reference = std::clamp(reference, 0, originalSize);

    int removedBeforeReference = 0;
    for (int i : sorted)
      if (i < reference) ++removedBeforeReference;

    int insertPos = reference - removedBeforeReference;
    insertPos = std::clamp(insertPos, 0, (int)v.size());

    v.insert(v.begin() + insertPos, std::make_move_iterator(moveItems.begin()),
             std::make_move_iterator(moveItems.end()));

    std::set<int> moveIndices{};
    for (int i = 0; i < (int)moveItems.size(); i++)
      moveIndices.insert(insertPos + i);

    return moveIndices;
  }
}
