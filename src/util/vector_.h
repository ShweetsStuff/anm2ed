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

  template <typename T> int find_index(std::vector<T>& v, T& value)
  {
    auto it = std::find(v.begin(), v.end(), value);
    if (it == v.end()) return -1;
    return (int)(std::distance(v.begin(), it));
  }

  template <typename T> bool in_bounds(std::vector<T>& v, int index) { return index >= 0 && index < (int)v.size(); }
  template <typename T> void clamp_in_bounds(std::vector<T>& v, int& index)
  {
    index = std::clamp(index, 0, (int)v.size() - 1);
  }

  template <typename T> int move_index(std::vector<T>& v, int source, int dest)
  {
    auto size = (int)(v.size());
    if (source < 0 || source >= size) return -1;
    dest = std::clamp(dest, 0, size - 1);

    if (source == dest) return dest;

    auto isInsertAfter = source < dest;

    T item = std::move(v[source]);
    v.erase(v.begin() + source);

    if (dest > source) --dest; // destination shifts when removing earlier slot
    if (isInsertAfter) ++dest; // drop after original target

    dest = std::clamp(dest, 0, (int)(v.size()));
    v.insert(v.begin() + dest, std::move(item));
    return dest;
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
