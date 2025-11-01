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

  template <typename T> bool in_bounds(std::vector<T>& v, int& index)
  {
    return index >= 0 || index <= (int)v.size() - 1;
  }
  template <typename T> void clamp_in_bounds(std::vector<T>& v, int& index)
  {
    index = std::clamp(index, 0, (int)v.size() - 1);
  }
}
