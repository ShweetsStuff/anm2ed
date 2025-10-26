#include "snapshots.h"

namespace anm2ed::snapshots
{
  bool SnapshotStack::is_empty()
  {
    return top == 0;
  }

  void SnapshotStack::push(Snapshot& snapshot)
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

  void Snapshots::push(const anm2::Anm2& anm2, anm2::Reference reference, const std::string& message)
  {
    Snapshot snapshot = {anm2, reference, message};
    undoStack.push(snapshot);
    redoStack.clear();
  }

  void Snapshots::undo(anm2::Anm2& anm2, anm2::Reference& reference, std::string& message)
  {
    if (auto current = undoStack.pop())
    {
      Snapshot snapshot = {anm2, reference, message};
      redoStack.push(snapshot);
      anm2 = current->anm2;
      reference = current->reference;
      message = current->message;
    }
  }

  void Snapshots::redo(anm2::Anm2& anm2, anm2::Reference& reference, std::string& message)
  {
    if (auto current = redoStack.pop())
    {
      Snapshot snapshot = {anm2, reference, message};
      undoStack.push(snapshot);
      anm2 = current->anm2;
      reference = current->reference;
      message = current->message;
    }
  }

  void Snapshots::reset()
  {
    undoStack.clear();
    redoStack.clear();
  }
};
