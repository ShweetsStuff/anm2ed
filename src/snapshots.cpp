#include "snapshots.hpp"

#include <algorithm>
#include <utility>

#include <glm/common.hpp>

namespace anm2ed::snapshots
{
  bool is_vec2_equal(const glm::vec2& left, const glm::vec2& right) { return glm::all(glm::equal(left, right)); }

  bool is_vec3_equal(const glm::vec3& left, const glm::vec3& right) { return glm::all(glm::equal(left, right)); }

  bool is_vec4_equal(const glm::vec4& left, const glm::vec4& right) { return glm::all(glm::equal(left, right)); }

  bool is_playback_equal(const Playback& left, const Playback& right)
  {
    return left.time == right.time && left.isPlaying == right.isPlaying && left.isFinished == right.isFinished;
  }

  bool is_storage_equal(const Storage& left, const Storage& right)
  {
    return left.reference == right.reference && left.hovered == right.hovered &&
           left.labelsString == right.labelsString && left.ids == right.ids &&
           static_cast<const std::set<int>&>(left.selection) == static_cast<const std::set<int>&>(right.selection) &&
           left.references == right.references;
  }

  template <typename T> bool is_value_equal(const T& left, const T& right) { return left == right; }

  bool is_value_equal(const Playback& left, const Playback& right) { return is_playback_equal(left, right); }

  bool is_value_equal(const Storage& left, const Storage& right) { return is_storage_equal(left, right); }

  bool is_element_shallow_equal(const Element& left, const Element& right)
  {
    return left.type == right.type && left.tag == right.tag && left.name == right.name &&
           left.createdBy == right.createdBy && left.createdOn == right.createdOn && left.path == right.path &&
           left.vertex == right.vertex && left.fragment == right.fragment && left.binding == right.binding &&
           left.value == right.value && left.defaultAnimation == right.defaultAnimation && left.id == right.id &&
           left.layerId == right.layerId && left.nullId == right.nullId && left.spritesheetId == right.spritesheetId &&
           left.fps == right.fps && left.version == right.version && left.frameNum == right.frameNum &&
           left.duration == right.duration && left.atFrame == right.atFrame && left.eventId == right.eventId &&
           left.regionId == right.regionId && left.shaderId == right.shaderId && left.soundId == right.soundId &&
           left.groupId == right.groupId && left.index == right.index && left.isLoop == right.isLoop &&
           left.isVisible == right.isVisible && left.isShowRect == right.isShowRect &&
           left.isExpanded == right.isExpanded && left.isEnabled == right.isEnabled &&
           left.interpolation == right.interpolation && left.origin == right.origin &&
           left.rotation == right.rotation && left.soundIds == right.soundIds &&
           is_vec2_equal(left.pivot, right.pivot) && is_vec2_equal(left.crop, right.crop) &&
           is_vec2_equal(left.position, right.position) && is_vec2_equal(left.size, right.size) &&
           is_vec2_equal(left.scale, right.scale) && is_vec2_equal(left.shear, right.shear) &&
           is_vec3_equal(left.colorOffset, right.colorOffset) && is_vec4_equal(left.tint, right.tint);
  }

  bool is_element_recurseable(const Element& left, const Element& right)
  {
    if (left.type != right.type) return false;
    if (left.children.size() != right.children.size()) return false;

    for (size_t i = 0; i < left.children.size(); ++i)
      if (left.children[i].type != right.children[i].type) return false;

    return true;
  }

  bool is_element_equal(const Element& left, const Element& right)
  {
    if (!is_element_shallow_equal(left, right)) return false;
    if (left.children.size() != right.children.size()) return false;

    for (size_t i = 0; i < left.children.size(); ++i)
      if (!is_element_equal(left.children[i], right.children[i])) return false;

    return true;
  }

  void element_steps_build(std::vector<SnapshotElementStep>& out, const Element& before, const Element& after,
                           std::vector<int>& path)
  {
    if (!is_element_recurseable(before, after) || !is_element_shallow_equal(before, after))
    {
      out.push_back({path, before, after});
      return;
    }

    for (size_t i = 0; i < before.children.size(); ++i)
    {
      path.push_back((int)i);
      element_steps_build(out, before.children[i], after.children[i], path);
      path.pop_back();
    }
  }

  template <class E> ElementPointer<E> element_get(E& root, const std::vector<int>& path)
  {
    auto element = &root;
    for (auto index : path)
    {
      if (index < 0 || index >= (int)element->children.size()) return nullptr;
      element = &element->children[index];
    }
    return element;
  }

  SnapshotAnm2Step anm2_step_make(const Anm2& before, const Anm2& after)
  {
    SnapshotAnm2Step step{};
    if (before.isValid != after.isValid) step.isValid = SnapshotStepValue<bool>{before.isValid, after.isValid};

    std::vector<int> path{};
    element_steps_build(step.elements, before.root, after.root, path);
    return step;
  }

  void step_state_seed(SnapshotStep& step, const Snapshot& snapshot)
  {
#define X(type, name) step.name = SnapshotStepValue<type>{snapshot.name, {}};
    SNAPSHOT_STEP_STATE_FIELDS
#undef X
  }

  SnapshotStep step_anm2_make(const Snapshot& snapshot)
  {
    SnapshotStep step{};
    step.anm2.isValid = SnapshotStepValue<bool>{snapshot.anm2.isValid, {}};
    step.anm2.elements.push_back({{}, snapshot.anm2.root, {}});
    step_state_seed(step, snapshot);
    return step;
  }

  Reference focus_get(const Snapshot& snapshot)
  {
    return selection_focus_get(snapshot.selection, UidIndex(snapshot.anm2));
  }

  void selection_follow(Snapshot& snapshot, Reference before)
  {
    snapshot.anm2.uids_repair();
    ::anm2ed::selection_follow(snapshot.selection, snapshot.anm2, UidIndex(snapshot.anm2), before);
  }
}

namespace anm2ed
{
  int SnapshotStack::maxSize = snapshots::MAX;

  bool SnapshotAnm2Step::is_empty() const { return !isValid && elements.empty(); }

  void SnapshotAnm2Step::apply(Anm2& anm2, SnapshotStepDirection direction) const
  {
    if (isValid) anm2.isValid = direction == SnapshotStepDirection::UNDO ? isValid->undo : isValid->redo;

    for (auto& elementStep : elements)
      if (auto* element = snapshots::element_get(anm2.root, elementStep.path); element)
        *element = direction == SnapshotStepDirection::UNDO ? elementStep.undo : elementStep.redo;
  }

  bool SnapshotStep::is_empty() const
  {
    if (!anm2.is_empty()) return false;

#define X(type, name)                                                                                                  \
  if (this->name) return false;
    SNAPSHOT_STEP_STATE_FIELDS
#undef X

    return true;
  }

  void SnapshotStep::apply(Snapshot& snapshot, SnapshotStepDirection direction) const
  {
    snapshot.message = message;
    anm2.apply(snapshot.anm2, direction);

#define X(type, name)                                                                                                  \
  if (this->name) snapshot.name = direction == SnapshotStepDirection::UNDO ? this->name->undo : this->name->redo;
    SNAPSHOT_STEP_STATE_FIELDS
#undef X
  }

  bool SnapshotStack::is_empty() { return stack.empty(); }

  void SnapshotStack::push(SnapshotStep step)
  {
    if (maxSize <= 0)
    {
      stack.clear();
      return;
    }
    if ((int)stack.size() >= maxSize) stack.pop_front();
    stack.push_back(std::move(step));
  }

  std::optional<SnapshotStep> SnapshotStack::pop()
  {
    if (is_empty()) return std::nullopt;
    auto snapshot = std::move(stack.back());
    stack.pop_back();
    return snapshot;
  }

  void SnapshotStack::clear() { stack.clear(); }

  void SnapshotStack::trim_to_limit()
  {
    if (maxSize <= 0)
    {
      clear();
      return;
    }

    while ((int)stack.size() > maxSize)
      stack.pop_front();
  }

  void SnapshotStack::max_size_set(int value) { maxSize = std::max(0, value); }

  int SnapshotStack::max_size_get() { return maxSize; }

  void Snapshots::step_push(const std::string& message, SnapshotStep step)
  {
    if (pendingStep || pendingFocus) commit();
    pendingFocus = snapshots::focus_get(current);
    current.message = message;
    step.message = message;
    pendingStep = step.is_empty() ? std::nullopt : std::optional<SnapshotStep>(std::move(step));
  }

  void Snapshots::push(const std::string& message) { step_push(message, snapshots::step_anm2_make(current)); }

  void Snapshots::commit()
  {
    const auto& snapshot = current;
    if (pendingFocus) snapshots::selection_follow(current, *std::exchange(pendingFocus, std::nullopt));
    if (!pendingStep) return;

    auto step = std::move(*pendingStep);
    pendingStep.reset();

    Anm2 before{};
    before.isValid = step.anm2.isValid ? step.anm2.isValid->undo : snapshot.anm2.isValid;
    before.root = std::move(step.anm2.elements.front().undo);
    step.anm2 = snapshots::anm2_step_make(before, snapshot.anm2);

    if (step.anm2.isValid)
    {
      step.anm2.isValid->redo = snapshot.anm2.isValid;
      if (step.anm2.isValid->undo == step.anm2.isValid->redo) step.anm2.isValid.reset();
    }

    for (auto it = step.anm2.elements.begin(); it != step.anm2.elements.end();)
    {
      auto* element = snapshots::element_get(snapshot.anm2.root, it->path);
      if (!element)
      {
        it = step.anm2.elements.erase(it);
        continue;
      }

      it->redo = *element;
      if (snapshots::is_element_equal(it->undo, it->redo))
      {
        it = step.anm2.elements.erase(it);
        continue;
      }

      ++it;
    }

#define X(type, name)                                                                                                  \
  if (step.name)                                                                                                       \
  {                                                                                                                    \
    step.name->redo = snapshot.name;                                                                                   \
    if (snapshots::is_value_equal(step.name->undo, step.name->redo)) step.name.reset();                                \
  }
    SNAPSHOT_STEP_STATE_FIELDS
#undef X

    if (step.is_empty()) return;

    undoStack.push(std::move(step));
    redoStack.clear();
  }

  bool Snapshots::undo()
  {
    pendingStep.reset();
    pendingFocus.reset();
    if (auto step = undoStack.pop())
    {
      auto focus = snapshots::focus_get(current);
      step->apply(current, SnapshotStepDirection::UNDO);
      snapshots::selection_follow(current, focus);
      redoStack.push(std::move(*step));
      return true;
    }
    return false;
  }

  bool Snapshots::redo()
  {
    pendingStep.reset();
    pendingFocus.reset();
    if (auto step = redoStack.pop())
    {
      auto focus = snapshots::focus_get(current);
      step->apply(current, SnapshotStepDirection::REDO);
      snapshots::selection_follow(current, focus);
      undoStack.push(std::move(*step));
      return true;
    }
    return false;
  }

  void Snapshots::reset()
  {
    undoStack.clear();
    redoStack.clear();
    pendingStep.reset();
    pendingFocus.reset();
  }

  void Snapshots::apply_limit()
  {
    undoStack.trim_to_limit();
    redoStack.trim_to_limit();
  }
}
