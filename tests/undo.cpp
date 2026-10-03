#include "common.hpp"

#include <random>

#include "edit/edit.hpp"
#include "snapshots.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

constexpr int WALK_STEPS = 150;
constexpr int WALK_SEED = 1234;
constexpr int UNDO_LIMIT = 1000;

std::vector<Reference> references_all_get(const model::Model& anm2, bool isFrames)
{
  model::UidIndex index(anm2);
  std::vector<Reference> result{};
  auto add = [&](std::uint64_t uid)
  {
    auto reference = index.reference_get(uid);
    if (!reference || reference->itemType == NONE || reference->itemType == TRIGGER) return;
    if (isFrames == (reference->frameIndex >= 0)) result.push_back(*reference);
  };
  for (auto [i, animation] : anm2.animations_get())
    model::animation_tracks_each(*animation,
                                 [&](const model::Track& track)
                                 {
                                   add(track.uid);
                                   for (const auto& frame : track.frames)
                                     add(frame.uid);
                                 });
  return result;
}

// One random edit on randomly chosen live elements; returns false when nothing applicable exists.
bool edit_random_apply(model::Model& anm2, std::mt19937& random)
{
  auto frames = references_all_get(anm2, true);
  auto tracks = references_all_get(anm2, false);
  if (frames.empty() || tracks.empty()) return false;
  auto pick = [&](const auto& items)
  { return items[std::uniform_int_distribution<int>(0, (int)items.size() - 1)(random)]; };
  auto frame = pick(frames);
  auto other = pick(frames);

  switch (std::uniform_int_distribution<int>(0, 8)(random))
  {
    case 0:
      edit::frame_insert(anm2, frame, 0);
      break;
    case 1:
      if (frames.size() > 4) edit::frames_delete(anm2, {frame, other});
      break;
    case 2:
      edit::frames_duplicate(anm2, {frame, other}, frame);
      break;
    case 3:
      edit::frames_reverse(anm2, {frame, other});
      break;
    case 4:
      edit::frames_bake(anm2, {frame}, 1, false, false);
      break;
    case 5:
      edit::frame_split(anm2, frame, 1);
      break;
    case 6:
      if (frame.itemType != ROOT) edit::frames_move(anm2, {frame}, pick(tracks), 0);
      break;
    case 7:
      edit::animation_add(anm2, 0, -1, frame.animationIndex, "Walk");
      break;
    default:
      edit::animations_move(anm2, {frame.animationIndex}, {}, {.animationIndex = 0});
      break;
  }
  return true;
}

TEST_CASE("undo and redo restore every state of a random edit walk")
{
  SnapshotStack::max_size_set(UNDO_LIMIT);
  for (auto name :
       {"02_items.anm2", "03a_groups_nested.anm2", "04_group_root_transform.anm2", "07_special_interpolation.anm2"})
  {
    INFO("fixture: ", name);
    Snapshots snapshots{};
    snapshots.current.model = model_load(file_load(CORPUS_DIR / name));
    std::mt19937 random(WALK_SEED);

    std::vector<std::uint64_t> hashes{model::model_hash(snapshots.current.model)};
    for (int step = 0; step < WALK_STEPS; ++step)
    {
      snapshots.push("Edit");
      if (!edit_random_apply(snapshots.current.model, random)) break;
      auto count = snapshots.undoStack.size();
      snapshots.commit();
      if (snapshots.undoStack.size() != count) hashes.push_back(model::model_hash(snapshots.current.model));
    }
    REQUIRE(hashes.size() > 1);

    for (auto it = hashes.rbegin() + 1; it != hashes.rend(); ++it)
    {
      REQUIRE(snapshots.undo());
      CHECK(model::model_hash(snapshots.current.model) == *it);
    }
    CHECK_FALSE(snapshots.undo());

    for (auto it = hashes.begin() + 1; it != hashes.end(); ++it)
    {
      REQUIRE(snapshots.redo());
      CHECK(model::model_hash(snapshots.current.model) == *it);
    }
  }
}

TEST_CASE("selection follows its frame through an edit and its undo")
{
  Snapshots snapshots{};
  auto& current = snapshots.current;
  current.model = model_load(file_load(CORPUS_DIR / "02_items.anm2"));
  selection_focus_set(current.selection, current.model, {0, LAYER, 0, 1});
  selection_references_set(current.selection, current.model, SelectionKind::FRAMES, {{0, LAYER, 0, 1}});

  snapshots.push("Insert");
  edit::frame_insert(current.model, {0, LAYER, 0, -1}, 0);
  auto& track = *current.model.track_edit(Reference{0, LAYER, 0});
  std::rotate(track.frames.begin(), track.frames.end() - 1, track.frames.end());
  snapshots.commit();
  model::UidIndex index(current.model);
  CHECK(selection_focus_get(current.selection, index) == Reference{0, LAYER, 0, 2});
  CHECK(selection_references_get(current.selection, index, SelectionKind::FRAMES) ==
        std::set<Reference>{{0, LAYER, 0, 2}});

  REQUIRE(snapshots.undo());
  CHECK(selection_focus_get(current.selection, model::UidIndex(current.model)) == Reference{0, LAYER, 0, 1});
}
