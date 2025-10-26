#pragma once

#include "anm2.h"

namespace anm2ed::snapshots
{
  constexpr auto ACTION = "Action";
  constexpr auto MAX = 100;

  class Snapshot
  {
  public:
    anm2::Anm2 anm2{};
    anm2::Reference reference{};
    std::string message = ACTION;
  };

  class SnapshotStack
  {
  public:
    Snapshot snapshots[MAX];
    int top{};

    bool is_empty();
    void push(Snapshot& snapshot);
    Snapshot* pop();
    void clear();
  };

  class Snapshots
  {
  public:
    SnapshotStack undoStack{};
    SnapshotStack redoStack{};

    Snapshot* get();
    void push(const anm2::Anm2&, anm2::Reference, const std::string&);
    void undo(anm2::Anm2& anm2, anm2::Reference& reference, std::string&);
    void redo(anm2::Anm2& anm2, anm2::Reference& reference, std::string&);
    void reset();
  };
}
