#include "snapshots.h"

namespace anm2ed::snapshots
{
  namespace
  {
    void textures_ensure(anm2::Anm2& anm2)
    {
      for (auto& [id, spritesheet] : anm2.content.spritesheets)
        spritesheet.texture.ensure_pixels();
    }
  }

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
    textures_ensure(const_cast<anm2::Anm2&>(anm2));
    Snapshot snapshot = {anm2, reference, message};
    undoStack.push(snapshot);
    redoStack.clear();
  }

  void Snapshots::undo(anm2::Anm2& anm2, anm2::Reference& reference, std::string& message)
  {
    if (auto current = undoStack.pop())
    {
      textures_ensure(anm2);
      Snapshot snapshot = {anm2, reference, message};
      redoStack.push(snapshot);
      textures_ensure(current->anm2);
      anm2 = current->anm2;
      reference = current->reference;
      message = current->message;
    }
  }

  void Snapshots::redo(anm2::Anm2& anm2, anm2::Reference& reference, std::string& message)
  {
    if (auto current = redoStack.pop())
    {
      textures_ensure(anm2);
      Snapshot snapshot = {anm2, reference, message};
      undoStack.push(snapshot);
      textures_ensure(current->anm2);
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
