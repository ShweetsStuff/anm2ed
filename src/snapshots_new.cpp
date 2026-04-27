#include "snapshots_new.hpp"

#include <algorithm>

using namespace anm2ed::snapshots_new;

namespace anm2ed
{
  int SnapshotStackNew::maxSize = snapshots_new::MAX;

  bool SnapshotStackNew::is_empty() { return stack.empty(); }

  void SnapshotStackNew::push(const SnapshotNew& snapshot)
  {
    if (maxSize <= 0)
    {
      stack.clear();
      return;
    }
    if ((int)stack.size() >= maxSize) stack.pop_front();
    stack.push_back(snapshot);
  }

  std::optional<SnapshotNew> SnapshotStackNew::pop()
  {
    if (is_empty()) return std::nullopt;
    auto snapshot = stack.back();
    stack.pop_back();
    return snapshot;
  }

  void SnapshotStackNew::clear() { stack.clear(); }

  void SnapshotStackNew::trim_to_limit()
  {
    if (maxSize <= 0)
    {
      clear();
      return;
    }

    while ((int)stack.size() > maxSize)
      stack.pop_front();
  }

  void SnapshotStackNew::max_size_set(int value) { maxSize = std::max(0, value); }

  int SnapshotStackNew::max_size_get() { return maxSize; }

  SnapshotNew* SnapshotsNew::get() { return &current; }

  void SnapshotsNew::push(const SnapshotNew& snapshot)
  {
    undoStack.push(snapshot);
    redoStack.clear();
  }

  void SnapshotsNew::undo()
  {
    if (auto snapshot = undoStack.pop())
    {
      redoStack.push(current);
      current = *snapshot;
    }
  }

  void SnapshotsNew::redo()
  {
    if (auto snapshot = redoStack.pop())
    {
      undoStack.push(current);
      current = *snapshot;
    }
  }

  void SnapshotsNew::reset()
  {
    undoStack.clear();
    redoStack.clear();
  }

  void SnapshotsNew::apply_limit()
  {
    undoStack.trim_to_limit();
    redoStack.trim_to_limit();
  }
}
