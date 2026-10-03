#pragma once

#include <deque>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include "anm2/anm2.hpp"
#include "audio_data.hpp"
#include "image.hpp"
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
  enum class SnapshotStepDirection
  {
    UNDO,
    REDO
  };

  template <typename T> struct SnapshotStepValue
  {
    T undo{};
    T redo{};
  };

  struct SnapshotElementStep
  {
    std::vector<int> path{};
    Element undo{};
    Element redo{};
  };

  struct SnapshotAnm2Step
  {
    std::optional<SnapshotStepValue<bool>> isValid{};
    std::vector<SnapshotElementStep> elements{};

    bool is_empty() const;
    void apply(Anm2&, SnapshotStepDirection) const;
  };

  class Snapshot
  {
  public:
    Playback playback{};
    Storage event{};
    Storage layer{};
    Storage merge{};
    Storage null{};
    Storage region{};
    Storage shader{};
    Storage sound{};
    Storage spritesheet{};
    std::map<int, resource::Image> textures{};
    std::map<int, resource::AudioData> sounds{};
    Anm2 anm2{};
    Selection selection{};
    float frameTime{};
    std::string message = snapshots::ACTION;
  };

  using SnapshotTextureMap = std::map<int, resource::Image>;
  using SnapshotSoundMap = std::map<int, resource::AudioData>;

#define SNAPSHOT_STEP_STATE_FIELDS                                                                                     \
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
  X(float, frameTime)

#define SNAPSHOT_STEP_RESOURCE_FIELDS                                                                                  \
  X(SnapshotTextureMap, textures)                                                                                      \
  X(SnapshotSoundMap, sounds)

  struct SnapshotStep
  {
    std::string message = snapshots::ACTION;
    SnapshotAnm2Step anm2{};

#define X(type, name) std::optional<SnapshotStepValue<type>> name{};
    SNAPSHOT_STEP_STATE_FIELDS
    SNAPSHOT_STEP_RESOURCE_FIELDS
#undef X

    bool is_empty() const;
    void apply(Snapshot&, SnapshotStepDirection) const;
  };

  class SnapshotStack
  {
  public:
    SnapshotStack() = default;

    bool is_empty();
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
    std::optional<SnapshotStep> pendingStep{};
    std::optional<Reference> pendingFocus{};

    void push(const std::string&, bool = false);
    void step_push(const std::string&, SnapshotStep);
    void commit();
    bool undo();
    bool redo();
    void reset();
    void apply_limit();
  };
}
