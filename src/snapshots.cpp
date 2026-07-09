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
    return left.reference == right.reference && left.hovered == right.hovered && left.labelsString == right.labelsString &&
           left.ids == right.ids && static_cast<const std::set<int>&>(left.selection) ==
                                       static_cast<const std::set<int>&>(right.selection) &&
           left.references == right.references;
  }

  bool is_texture_equal(const resource::Texture& left, const resource::Texture& right)
  {
    return left.size == right.size && left.filter == right.filter && left.channels == right.channels &&
           left.pixels == right.pixels;
  }

  bool is_audio_equal(const resource::Audio& left, const resource::Audio& right)
  {
    return const_cast<resource::Audio&>(left).is_valid() == const_cast<resource::Audio&>(right).is_valid();
  }

  template <typename T, typename Compare>
  bool is_map_equal(const std::map<int, T>& left, const std::map<int, T>& right, Compare compare)
  {
    if (left.size() != right.size()) return false;

    auto leftIt = left.begin();
    auto rightIt = right.begin();
    while (leftIt != left.end() && rightIt != right.end())
    {
      if (leftIt->first != rightIt->first) return false;
      if (!compare(leftIt->second, rightIt->second)) return false;
      ++leftIt;
      ++rightIt;
    }

    return leftIt == left.end() && rightIt == right.end();
  }

  template <typename T> bool is_value_equal(const T& left, const T& right) { return left == right; }

  bool is_value_equal(const Playback& left, const Playback& right) { return is_playback_equal(left, right); }

  bool is_value_equal(const Storage& left, const Storage& right) { return is_storage_equal(left, right); }

  bool is_value_equal(const std::map<int, resource::Texture>& left, const std::map<int, resource::Texture>& right)
  {
    return is_map_equal(left, right, is_texture_equal);
  }

  bool is_value_equal(const std::map<int, resource::Audio>& left, const std::map<int, resource::Audio>& right)
  {
    return is_map_equal(left, right, is_audio_equal);
  }

  bool is_element_shallow_equal(const Element& left, const Element& right)
  {
    return left.type == right.type && left.tag == right.tag && left.name == right.name &&
           left.createdBy == right.createdBy && left.createdOn == right.createdOn && left.path == right.path &&
           left.vertex == right.vertex && left.fragment == right.fragment && left.binding == right.binding &&
           left.value == right.value && left.defaultAnimation == right.defaultAnimation && left.id == right.id &&
           left.layerId == right.layerId && left.nullId == right.nullId &&
           left.spritesheetId == right.spritesheetId && left.fps == right.fps && left.version == right.version &&
           left.frameNum == right.frameNum && left.duration == right.duration && left.atFrame == right.atFrame &&
           left.eventId == right.eventId && left.regionId == right.regionId && left.soundId == right.soundId &&
           left.groupId == right.groupId && left.index == right.index && left.isLoop == right.isLoop &&
           left.isVisible == right.isVisible && left.isShowRect == right.isShowRect && left.isExpanded == right.isExpanded &&
           left.isEnabled == right.isEnabled && left.interpolation == right.interpolation && left.origin == right.origin &&
           left.rotation == right.rotation && left.soundIds == right.soundIds && is_vec2_equal(left.pivot, right.pivot) &&
           is_vec2_equal(left.crop, right.crop) && is_vec2_equal(left.position, right.position) &&
           is_vec2_equal(left.size, right.size) && is_vec2_equal(left.scale, right.scale) &&
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

  Element* element_get(Element& root, const std::vector<int>& path)
  {
    auto* element = &root;
    for (auto index : path)
    {
      if (index < 0 || index >= (int)element->children.size()) return nullptr;
      element = &element->children[index];
    }
    return element;
  }

  const Element* element_get(const Element& root, const std::vector<int>& path)
  {
    auto* element = &root;
    for (auto index : path)
    {
      if (index < 0 || index >= (int)element->children.size()) return nullptr;
      element = &element->children[index];
    }
    return element;
  }

  bool element_path_get(const Element& element, const Element* target, std::vector<int>& path)
  {
    for (size_t i = 0; i < element.children.size(); ++i)
    {
      path.push_back((int)i);
      if (&element.children[i] == target) return true;
      if (element_path_get(element.children[i], target, path)) return true;
      path.pop_back();
    }
    return false;
  }

  std::optional<std::vector<int>> frame_path_get(const Anm2& anm2, Reference frameReference)
  {
    auto frame = anm2.element_get(frameReference);
    if (!frame) return std::nullopt;

    std::vector<int> path{};
    if (!element_path_get(anm2.root, frame, path)) return std::nullopt;
    return path;
  }

  std::optional<std::vector<int>> region_path_get(const Anm2& anm2, int spritesheetId, int regionId)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
    if (!spritesheet) return std::nullopt;
    auto region = element_child_id_get(*spritesheet, ElementType::REGION, regionId);
    if (!region) return std::nullopt;

    std::vector<int> path{};
    if (!element_path_get(anm2.root, region, path)) return std::nullopt;
    return path;
  }

  void step_state_seed(SnapshotStep& step, const Snapshot& snapshot)
  {
#define X(type, name) step.name = SnapshotStepValue<type>{snapshot.name, {}};
    SNAPSHOT_STEP_STATE_FIELDS
#undef X
  }

  void step_textures_seed(SnapshotStep& step, const Snapshot& snapshot)
  {
    step.textures = SnapshotStepValue<SnapshotTextureMap>{snapshot.textures, {}};
    step.overlayTextures = SnapshotStepValue<SnapshotOverlayTextureMap>{snapshot.overlayTextures, {}};
  }

  SnapshotStep step_anm2_seed(const Snapshot& snapshot, const std::string& message)
  {
    SnapshotStep step{};
    step.message = message;
    step.anm2.isValid = SnapshotStepValue<bool>{snapshot.anm2.isValid, {}};
    step.anm2.elements.push_back({{}, snapshot.anm2.root, {}});
    step_state_seed(step, snapshot);
    return step;
  }

  void frames_step_add(SnapshotStep& step, const Snapshot& snapshot, Reference frameReference)
  {
    auto path = frame_path_get(snapshot.anm2, frameReference);
    if (!path) return;
    for (auto& elementStep : step.anm2.elements)
      if (elementStep.path == *path) return;

    auto frame = snapshot.anm2.element_get(frameReference);
    if (!frame) return;

    step.anm2.elements.push_back({std::move(*path), *frame, {}});
  }

  void regions_step_add(SnapshotStep& step, const Snapshot& snapshot, int spritesheetId, int regionId)
  {
    auto path = region_path_get(snapshot.anm2, spritesheetId, regionId);
    if (!path) return;
    for (auto& elementStep : step.anm2.elements)
      if (elementStep.path == *path) return;

    auto spritesheet = snapshot.anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
    if (!spritesheet) return;
    auto region = element_child_id_get(*spritesheet, ElementType::REGION, regionId);
    if (!region) return;

    step.anm2.elements.push_back({std::move(*path), *region, {}});
  }

  SnapshotAnm2Step anm2_step_make(const Anm2& before, const Anm2& after)
  {
    SnapshotAnm2Step step{};
    if (before.isValid != after.isValid) step.isValid = SnapshotStepValue<bool>{before.isValid, after.isValid};

    std::vector<int> path{};
    element_steps_build(step.elements, before.root, after.root, path);
    return step;
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
    SNAPSHOT_STEP_RESOURCE_FIELDS
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
    SNAPSHOT_STEP_RESOURCE_FIELDS
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

  Snapshot* Snapshots::get() { return &current; }

  void Snapshots::anm2_push(const Snapshot& snapshot, const std::string& message)
  {
    pendingStep = snapshots::step_anm2_seed(snapshot, message);
  }

  void Snapshots::frames_push(const Snapshot& snapshot, const std::string& message,
                              const std::set<Reference>& frameReferences)
  {
    SnapshotStep step{};
    step.message = message;
    for (auto frameReference : frameReferences)
      snapshots::frames_step_add(step, snapshot, frameReference);

    pendingStep = step.is_empty() ? std::nullopt : std::optional<SnapshotStep>(std::move(step));
  }

  void Snapshots::regions_push(const Snapshot& snapshot, const std::string& message, int spritesheetId,
                               const std::set<int>& regionIds)
  {
    SnapshotStep step{};
    step.message = message;
    for (auto regionId : regionIds)
      snapshots::regions_step_add(step, snapshot, spritesheetId, regionId);

    pendingStep = step.is_empty() ? std::nullopt : std::optional<SnapshotStep>(std::move(step));
  }

  void Snapshots::textures_push(const Snapshot& snapshot, const std::string& message)
  {
    SnapshotStep step{};
    step.message = message;
    snapshots::step_textures_seed(step, snapshot);
    pendingStep = std::move(step);
  }

  void Snapshots::anm2_textures_push(const Snapshot& snapshot, const std::string& message)
  {
    auto step = snapshots::step_anm2_seed(snapshot, message);
    snapshots::step_textures_seed(step, snapshot);
    pendingStep = std::move(step);
  }

  void Snapshots::commit(const Snapshot& snapshot)
  {
    if (!pendingStep) return;

    auto step = std::move(*pendingStep);
    pendingStep.reset();

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
    if (step.name)                                                                                                     \
    {                                                                                                                  \
      step.name->redo = snapshot.name;                                                                                 \
      if (snapshots::is_value_equal(step.name->undo, step.name->redo)) step.name.reset();                             \
    }
    SNAPSHOT_STEP_STATE_FIELDS
    SNAPSHOT_STEP_RESOURCE_FIELDS
#undef X

    if (step.is_empty()) return;

    undoStack.push(std::move(step));
    redoStack.clear();
  }

  bool Snapshots::undo()
  {
    pendingStep.reset();
    if (auto step = undoStack.pop())
    {
      step->apply(current, SnapshotStepDirection::UNDO);
      redoStack.push(std::move(*step));
      return true;
    }
    return false;
  }

  bool Snapshots::redo()
  {
    pendingStep.reset();
    if (auto step = redoStack.pop())
    {
      step->apply(current, SnapshotStepDirection::REDO);
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
  }

  void Snapshots::apply_limit()
  {
    undoStack.trim_to_limit();
    redoStack.trim_to_limit();
  }
}
