#pragma once

#include "anm2/anm2.h"
#include "playback.h"
#include "storage.h"

namespace anm2ed::snapshots
{
  constexpr auto ACTION = "Action";
  constexpr auto MAX = 100;
};

namespace anm2ed
{
  class Snapshot
  {
  public:
    anm2::Anm2 anm2{};
    anm2::Reference reference{};
    float frameTime{};
    Playback playback{};
    Storage animation{};
    Storage merge{};
    Storage event{};
    Storage layer{};
    Storage null{};
    Storage sound{};
    Storage spritesheet{};
    Storage items{};
    Storage frames{};
    std::string message = snapshots::ACTION;
  };

  class SnapshotStack
  {
  public:
    Snapshot snapshots[snapshots::MAX];
    int top{};

    bool is_empty();
    void push(const Snapshot&);
    Snapshot* pop();
    void clear();
  };

  class Snapshots
  {
  public:
    SnapshotStack undoStack{};
    SnapshotStack redoStack{};
    Snapshot current{};

    Snapshot* get();
    void push(const Snapshot&);
    void undo();
    void redo();
    void reset();
  };
}
