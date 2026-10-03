#pragma once

#include <deque>
#include <map>
#include <optional>
#include <string>

#include "model/model.hpp"
#include "playback.hpp"
#include "selection.hpp"
#include "storage.hpp"

namespace anm2ed::snapshots
{
  constexpr auto ACTION = "Action";
  constexpr auto MAX = 20;
};

namespace anm2ed
{
  using AssetKeys = std::map<int, std::uint64_t>;

#define SNAPSHOT_STEP_STATE_FIELDS                                                                                     \
  X(model::Model, model)                                                                                               \
  X(Playback, playback)                                                                                                \
  X(Storage, event)                                                                                                    \
  X(Storage, layer)                                                                                                    \
  X(Storage, merge)                                                                                                    \
  X(Storage, null)                                                                                                     \
  X(Storage, region)                                                                                                   \
  X(Storage, shader)                                                                                                   \
  X(Storage, sound)                                                                                                    \
  X(Storage, spritesheet)                                                                                              \
  X(Selection, selection)                                                                                              \
  X(float, frameTime)                                                                                                  \
  X(AssetKeys, textures)                                                                                               \
  X(AssetKeys, sounds)

  // Everything undo restores. The model's animations are shared between snapshots and copied only when edited.
  struct Snapshot
  {
#define X(type, name) type name{};
    SNAPSHOT_STEP_STATE_FIELDS
#undef X
    std::string message = snapshots::ACTION;
  };

  // One undoable edit: the state before and after it; undo/redo restore only the fields the edit changed.
  struct SnapshotStep
  {
    Snapshot before{};
    Snapshot after{};
  };

  class SnapshotStack
  {
  public:
    bool is_empty();
    std::size_t size() const;
    void push(SnapshotStep);
    std::optional<SnapshotStep> pop();
    void clear();
    void trim_to_limit();

    static void max_size_set(int);
    static int max_size_get();

  private:
    static int maxSize;
    std::deque<SnapshotStep> stack;
  };

  class Snapshots
  {
  public:
    SnapshotStack undoStack{};
    SnapshotStack redoStack{};
    Snapshot current{};
    std::optional<Snapshot> pending{};
    std::optional<Reference> pendingFocus{};

    void push(const std::string&);
    void commit();
    bool undo();
    bool redo();
    void reset();
    void apply_limit();
  };
}
