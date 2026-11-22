#include "common.h"

#include <random>

#include "edit/edit.h"
#include "snapshots.h"

constexpr int WALK_STEPS = 150;
constexpr int WALK_SEED = 1234;
constexpr int UNDO_LIMIT = 1000;

static std::vector<Reference> references_all_get(const Model& anm2, bool isFrames)
{
    UidIndex index(anm2);
    std::vector<Reference> result;
    auto add = [&](std::uint64_t uid) {
        auto reference = index.reference_get(uid);
        if (!reference || reference->itemType == NONE || reference->itemType == TRIGGER)
            return;
        if (isFrames == (reference->frameIndex >= 0))
            result.push_back(*reference);
    };
    for (auto [i, animation] : anm2.animations_get())
        animation_tracks_each(*animation, [&](const Track& track) {
            add(track.uid);
            for (auto& frame : track.frames)
                add(frame.uid);
        });
    return result;
}

// One random edit on randomly chosen live elements. Returns false when nothing applicable exists.
static bool edit_random_apply(Model& anm2, std::mt19937& random)
{
    auto frames = references_all_get(anm2, true);
    auto tracks = references_all_get(anm2, false);
    if (frames.empty() || tracks.empty())
        return false;
    auto pick = [&](const auto& items) {
        return items[std::uniform_int_distribution<int>(0, (int)items.size() - 1)(random)];
    };
    auto frame = pick(frames);
    auto other = pick(frames);

    switch (std::uniform_int_distribution<int>(0, 8)(random))
    {
        case 0:
            frame_insert(anm2, frame, 0);
            break;
        case 1:
            if (frames.size() > 4)
                frames_delete(anm2, {frame, other});
            break;
        case 2:
            frames_duplicate(anm2, {frame, other}, frame);
            break;
        case 3:
            frames_reverse(anm2, {frame, other});
            break;
        case 4:
            frames_bake(anm2, {frame}, 1, {});
            break;
        case 5:
            frame_split(anm2, frame, 1);
            break;
        case 6:
            if (frame.itemType != ROOT)
                frames_move(anm2, {frame}, pick(tracks), 0);
            break;
        case 7:
            animation_add(anm2, 0, -1, frame.animationIndex, "Walk");
            break;
        default:
            animations_move(anm2, {frame.animationIndex}, {}, {.animationIndex = 0});
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
        Snapshots snapshots = {};
        Snapshot current = {};
        current.model = model_load(file_load(CORPUS_DIR / name));
        std::mt19937 random(WALK_SEED);

        std::vector<std::uint64_t> hashes{model_hash(current.model)};
        for (int step = 0; step < WALK_STEPS; ++step)
        {
            snapshots.push(current, "Edit");
            if (!edit_random_apply(current.model, random))
                break;
            auto count = snapshots.undoStack.size();
            snapshots.commit(current);
            if (snapshots.undoStack.size() != count)
                hashes.push_back(model_hash(current.model));
        }
        REQUIRE(hashes.size() > 1);

        for (auto it = hashes.rbegin() + 1; it != hashes.rend(); ++it)
        {
            REQUIRE(snapshots.undo(current));
            CHECK(model_hash(current.model) == *it);
        }
        CHECK_FALSE(snapshots.undo(current));

        for (auto it = hashes.begin() + 1; it != hashes.end(); ++it)
        {
            REQUIRE(snapshots.redo(current));
            CHECK(model_hash(current.model) == *it);
        }
    }
}

TEST_CASE("selection follows its frame through an edit and its undo")
{
    Snapshots snapshots = {};
    Snapshot current = {};
    current.model = model_load(file_load(CORPUS_DIR / "02_items.anm2"));
    selection_focus_set(current.selection, current.model, {0, LAYER, 0, 1});
    selection_references_set(current.selection, current.model, SelectionKind::FRAMES, {{0, LAYER, 0, 1}});

    snapshots.push(current, "Insert");
    frame_insert(current.model, {0, LAYER, 0, -1}, 0);
    auto& track = *current.model.track_edit(Reference{0, LAYER, 0});
    std::rotate(track.frames.begin(), track.frames.end() - 1, track.frames.end());
    snapshots.commit(current);
    UidIndex index(current.model);
    CHECK(selection_focus_get(current.selection, index) == Reference{0, LAYER, 0, 2});
    CHECK(selection_references_get(current.selection, index, SelectionKind::FRAMES) ==
          std::set<Reference>{{0, LAYER, 0, 2}});

    REQUIRE(snapshots.undo(current));
    CHECK(selection_focus_get(current.selection, UidIndex(current.model)) == Reference{0, LAYER, 0, 1});
}
