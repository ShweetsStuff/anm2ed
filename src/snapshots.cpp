#include "snapshots.h"

using namespace anm2ed::snapshots;

namespace anm2ed
{
  bool SnapshotStack::is_empty()
  {
    return top == 0;
  }

  void SnapshotStack::push(const Snapshot& snapshot)
  {
    if (top >= MAX)
    {
      for (int i = 0; i < MAX - 1; i++)
        snapshots[i] = snapshots[i + 1];
      top = MAX - 1;
    }
    snapshots[top++] = snapshot;
  }

  Snapshot* SnapshotStack::pop()
  {
    if (is_empty()) return nullptr;
    return &snapshots[--top];
  }

  void SnapshotStack::clear()
  {
    top = 0;
  }

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
};
