#include "snapshots.hpp"

#include <utility>

namespace anm2ed::snapshots
{
  template <typename T> bool is_value_equal(const T& left, const T& right) { return left == right; }

  bool is_value_equal(const model::Model& left, const model::Model& right)
  {
    return model::is_model_equal(left, right);
  }

  bool is_value_equal(const Playback& left, const Playback& right)
  {
    return left.time == right.time && left.isPlaying == right.isPlaying && left.isFinished == right.isFinished;
  }

  bool is_step_empty(const SnapshotStep& step)
  {
#define X(type, name)                                                                                                  \
  if (!is_value_equal(step.before.name, step.after.name)) return false;
    SNAPSHOT_STEP_STATE_FIELDS
#undef X
    return true;
  }

  void step_apply(Snapshot& current, const SnapshotStep& step, bool isUndo)
  {
    const auto& target = isUndo ? step.before : step.after;
#define X(type, name)                                                                                                  \
  if (!is_value_equal(step.before.name, step.after.name)) current.name = target.name;
    SNAPSHOT_STEP_STATE_FIELDS
#undef X
    current.message = step.after.message;
  }

  Reference focus_get(const Snapshot& snapshot)
  {
    return selection_focus_get(snapshot.selection, model::UidIndex(snapshot.model));
  }

  void selection_follow(Snapshot& snapshot, Reference before)
  {
    snapshot.model.uids_repair();
    ::anm2ed::selection_follow(snapshot.selection, snapshot.model, model::UidIndex(snapshot.model), before);
  }
}

namespace anm2ed
{
  int SnapshotStack::maxSize = snapshots::MAX;

  bool SnapshotStack::is_empty() { return stack.empty(); }
  std::size_t SnapshotStack::size() const { return stack.size(); }

  void SnapshotStack::push(SnapshotStep step)
  {
    stack.push_back(std::move(step));
    trim_to_limit();
  }

  std::optional<SnapshotStep> SnapshotStack::pop()
  {
    if (stack.empty()) return std::nullopt;
    auto step = std::move(stack.back());
    stack.pop_back();
    return step;
  }

  void SnapshotStack::clear() { stack.clear(); }

  void SnapshotStack::trim_to_limit()
  {
    while ((int)stack.size() > maxSize)
      stack.pop_front();
  }

  void SnapshotStack::max_size_set(int value) { maxSize = std::max(value, 0); }

  // Starts an undoable edit: remembers the state before it (cheap; animations are shared).
  void Snapshots::push(const std::string& message)
  {
    if (pending || pendingFocus) commit();
    pendingFocus = snapshots::focus_get(current);
    current.message = message;
    pending = current;
  }

  void Snapshots::commit()
  {
    if (pendingFocus) snapshots::selection_follow(current, *std::exchange(pendingFocus, std::nullopt));
    if (!pending) return;
    SnapshotStep step{std::move(*std::exchange(pending, std::nullopt)), current};
    if (snapshots::is_step_empty(step)) return;
    undoStack.push(std::move(step));
    redoStack.clear();
  }

  bool Snapshots::undo()
  {
    pending.reset();
    pendingFocus.reset();
    auto step = undoStack.pop();
    if (!step) return false;
    auto focus = snapshots::focus_get(current);
    snapshots::step_apply(current, *step, true);
    snapshots::selection_follow(current, focus);
    redoStack.push(std::move(*step));
    return true;
  }

  bool Snapshots::redo()
  {
    pending.reset();
    pendingFocus.reset();
    auto step = redoStack.pop();
    if (!step) return false;
    auto focus = snapshots::focus_get(current);
    snapshots::step_apply(current, *step, false);
    snapshots::selection_follow(current, focus);
    undoStack.push(std::move(*step));
    return true;
  }

  void Snapshots::reset()
  {
    undoStack.clear();
    redoStack.clear();
    pending.reset();
    pendingFocus.reset();
  }

  void Snapshots::apply_limit()
  {
    undoStack.trim_to_limit();
    redoStack.trim_to_limit();
  }
}
