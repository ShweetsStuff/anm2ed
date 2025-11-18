#include "snapshots.h"

#include <algorithm>

using namespace anm2ed::snapshots;

namespace anm2ed
{
  int SnapshotStack::maxSize = snapshots::MAX;

  bool SnapshotStack::is_empty() { return stack.empty(); }

  void SnapshotStack::push(const Snapshot& snapshot)
  {
    if (maxSize <= 0)
    {
      stack.clear();
      return;
    }
    if ((int)stack.size() >= maxSize) stack.pop_front();
    stack.push_back(snapshot);
  }

  std::optional<Snapshot> SnapshotStack::pop()
  {
    if (is_empty()) return std::nullopt;
    auto snapshot = stack.back();
    stack.pop_back();
    return snapshot;
  }

  void SnapshotStack::clear() { stack.clear(); }

  void SnapshotStack::trim_to_limit()
  {
    if (maxSize <= 0)
    {
      clear();
      return;
    }

    while ((int)stack.size() > maxSize)
      stack.pop_front();
  }

  void SnapshotStack::max_size_set(int value) { maxSize = std::max(0, value); }

  int SnapshotStack::max_size_get() { return maxSize; }

  void Snapshots::push(const Snapshot& snapshot)
  {
    undoStack.push(snapshot);
    redoStack.clear();
  }

  void Snapshots::undo()
  {
    if (auto snapshot = undoStack.pop())
    {
      redoStack.push(current);
      current = *snapshot;
    }
  }

  void Snapshots::redo()
  {
    if (auto snapshot = redoStack.pop())
    {
      undoStack.push(current);
      current = *snapshot;
    }
  }

  void Snapshots::reset()
  {
    undoStack.clear();
    redoStack.clear();
  }

  void Snapshots::apply_limit()
  {
    undoStack.trim_to_limit();
    redoStack.trim_to_limit();
  }
};
