#pragma once

#include <deque>
#include <optional>

#include "anm2_new/anm2.hpp"

namespace anm2ed::snapshots_new
{
  constexpr auto ACTION = "Action";
  constexpr auto MAX = 20;
}

namespace anm2ed
{
  class SnapshotNew
  {
  public:
    anm2_new::Anm2 anm2{};
    std::string message = snapshots_new::ACTION;
  };

  class SnapshotStackNew
  {
  public:
    SnapshotStackNew() = default;

    bool is_empty();
    void push(const SnapshotNew&);
    std::optional<SnapshotNew> pop();
    void clear();
    void trim_to_limit();

    static void max_size_set(int);
    static int max_size_get();

  private:
    static int maxSize;
    std::deque<SnapshotNew> stack;
  };

  class SnapshotsNew
  {
  public:
    SnapshotStackNew undoStack{};
    SnapshotStackNew redoStack{};
    SnapshotNew current{};

    SnapshotNew* get();
    void push(const SnapshotNew&);
    void undo();
    void redo();
    void reset();
    void apply_limit();
  };
}
