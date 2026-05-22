#pragma once

#include <deque>
#include <map>
#include <optional>
#include <set>

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
    Storage region{};
    Storage sound{};
    Storage spritesheet{};
    std::map<int, resource::Texture> textures{};
    std::map<int, resource::Audio> sounds{};
    Anm2 anm2{};
    Reference reference{};
    std::set<Reference> groupReferences{};
    float frameTime{};
    std::string message = snapshots::ACTION;

    Snapshot stack_copy_get() const;
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
    bool undo();
    bool redo();
    void reset();
    void apply_limit();
  };
}
