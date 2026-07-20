#pragma once

#include <deque>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include "anm2/anm2.hpp"
#include "audio.hpp"
#include "playback.hpp"
#include "storage.hpp"
#include "texture.hpp"

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
    Storage animation{};
    Storage event{};
    Storage frames{};
    Storage items{};
    Storage layer{};
    Storage merge{};
    Storage null{};
    Storage overlay{};
    Storage region{};
    Storage sound{};
    Storage spritesheet{};
    std::map<int, resource::Texture> textures{};
    std::map<int, resource::Texture> overlayTextures{};
    std::map<int, resource::Audio> sounds{};
    Anm2 anm2{};
    Reference reference{};
    std::set<Reference> groupReferences{};
    float frameTime{};
    std::string message = snapshots::ACTION;
  };

  using SnapshotTextureMap = std::map<int, resource::Texture>;
  using SnapshotOverlayTextureMap = std::map<int, resource::Texture>;
  using SnapshotSoundMap = std::map<int, resource::Audio>;

#define SNAPSHOT_STEP_STATE_FIELDS                                                                                     \
  X(Playback, playback)                                                                                                \
  X(Storage, animation)                                                                                                \
  X(Storage, event)                                                                                                    \
  X(Storage, frames)                                                                                                   \
  X(Storage, items)                                                                                                    \
  X(Storage, layer)                                                                                                    \
  X(Storage, merge)                                                                                                    \
  X(Storage, null)                                                                                                     \
  X(Storage, overlay)                                                                                                  \
  X(Storage, region)                                                                                                   \
  X(Storage, sound)                                                                                                    \
  X(Storage, spritesheet)                                                                                              \
  X(Reference, reference)                                                                                              \
  X(std::set<Reference>, groupReferences)                                                                              \
  X(float, frameTime)

#define SNAPSHOT_STEP_RESOURCE_FIELDS                                                                                  \
  X(SnapshotTextureMap, textures)                                                                                      \
  X(SnapshotOverlayTextureMap, overlayTextures)                                                                        \
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

    Snapshot* get();
    void anm2_push(const Snapshot&, const std::string&);
    void tracks_push(const Snapshot&, const std::string&, const std::set<Reference>&);
    void frames_push(const Snapshot&, const std::string&, const std::set<Reference>&);
    void regions_push(const Snapshot&, const std::string&, int, const std::set<int>&);
    void textures_push(const Snapshot&, const std::string&);
    void anm2_textures_push(const Snapshot&, const std::string&);
    void commit(const Snapshot&);
    bool undo();
    bool redo();
    void reset();
    void apply_limit();
  };
}
