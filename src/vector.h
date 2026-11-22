#pragma once

#include <algorithm>
#include <ranges>
#include <set>
#include <vector>

namespace anm2ed::util::vector
{
  template <typename T> T* find(std::vector<T>& v, int index)
  {
    return index >= 0 && index < (int)v.size() ? &v[index] : nullptr;
  }

  template <typename T> int find_index(const std::vector<T>& v, const T& value)
  {
    auto it = std::find(v.begin(), v.end(), value);
    if (it == v.end()) return -1;
    return (int)(std::distance(v.begin(), it));
  }

  template <typename T> bool in_bounds(std::vector<T>& v, int index) { return index >= 0 && index < (int)v.size(); }

  template <typename T>
  std::set<int> move_indices_to_position(std::vector<T>& v, const std::vector<int>& indices, int insertPos)
  {
    if (indices.empty()) return {};

    std::vector<int> sorted = indices;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    std::erase_if(sorted, [&](int i) { return i < 0 || i >= (int)v.size(); });
    if (sorted.empty()) return {};

    std::vector<T> moveItems;
    moveItems.reserve(sorted.size());
    for (int i : sorted)
      moveItems.push_back(std::move(v[i]));

    for (auto i : sorted | std::views::reverse)
      v.erase(v.begin() + i);

    insertPos = std::clamp(insertPos, 0, (int)v.size() + (int)sorted.size());
    for (int i : sorted)
      if (i < insertPos) --insertPos;
    insertPos = std::clamp(insertPos, 0, (int)v.size());

    v.insert(v.begin() + insertPos, std::make_move_iterator(moveItems.begin()),
             std::make_move_iterator(moveItems.end()));

    std::set<int> moveIndices{};
    for (int i = 0; i < (int)moveItems.size(); i++)
      moveIndices.insert(insertPos + i);

    return moveIndices;
  }
}
