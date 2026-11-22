#pragma once

#include <deque>
#include <map>
#include <optional>
#include <string>

#include "model/model.h"
#include "playback.h"
#include "selection.h"

constexpr auto SNAPSHOT_ACTION = "Action";
constexpr auto SNAPSHOT_MAX = 20;

using AssetKeys = std::map<int, std::uint64_t>;

#define SNAPSHOT_STEP_STATE_FIELDS                                                                                     \
    X(Model, model)                                                                                                    \
    X(Playback, playback)                                                                                              \
    X(Selection, selection)                                                                                            \
    X(float, frameTime)                                                                                                \
    X(AssetKeys, textures)                                                                                             \
    X(AssetKeys, sounds)

// Everything undo restores. The model's animations are shared between snapshots and copied only when edited.
struct Snapshot
{
#define X(type, name) type name{};
    SNAPSHOT_STEP_STATE_FIELDS
#undef X
    std::string message = SNAPSHOT_ACTION;
};

// One undoable edit: the state before and after it. Undo/redo restore only the fields the edit changed.
struct SnapshotStep
{
    Snapshot before = {};
    Snapshot after = {};
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

private:
    static int maxSize;
    std::deque<SnapshotStep> stack;
};

class Snapshots
{
public:
    SnapshotStack undoStack = {};
    SnapshotStack redoStack = {};
    std::optional<Snapshot> pending;
    std::optional<Reference> pendingFocus;

    void push(Snapshot&, const std::string&);
    void commit(Snapshot&);
    bool undo(Snapshot&);
    bool redo(Snapshot&);
    void reset();
    void apply_limit();
};
