#pragma once

#include <deque>
#include <optional>

#include "anm2/anm2.h"
#include "playback.h"
#include "storage.h"

namespace anm2ed::snapshots
{
  constexpr auto ACTION = "Action";
  constexpr auto MAX = 20;
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
    SnapshotStack() = default;

    bool is_empty();
    void push(const Snapshot&);
    std::optional<Snapshot> pop();
    void clear();
    void trim_to_limit();

    static void max_size_set(int);
    static int max_size_get();

  private:
    static int maxSize;
    std::deque<Snapshot> stack;
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
    void apply_limit();
  };
}
