#include "common.h"

#include "edit/edit.h"
#include "selection.h"

static Model fixture_load_with_uids(const char* name)
{
    return model_load(file_load(CORPUS_DIR / name));
}

// Applies an edit and follows the selection through it, the way a commit does.
void selection_edit(Model& anm2, Selection& selection, auto&& edit)
{
    auto before = selection_focus_get(selection, UidIndex(anm2));
    edit();
    anm2.uids_repair();
    selection_follow(selection, anm2, UidIndex(anm2), before);
}

TEST_CASE("uids are unique and non-zero after load")
{
    for (auto& fixture : fixtures_get())
    {
        auto anm2 = model_load(file_load(fixture));
        std::set<std::uint64_t> uids;
        bool isUnique = true;
        for (auto [index, animation] : anm2.animations_get())
            animation_tracks_each(*animation, [&](const Track& track) {
                isUnique &= uids.insert(track.uid).second;
                for (auto& frame : track.frames)
                    isUnique &= uids.insert(frame.uid).second;
            });
        INFO("fixture: ", fixture.filename().string());
        CHECK(isUnique);
    }
}

TEST_CASE("selected frames and focus follow inserts and moves")
{
    auto anm2 = fixture_load_with_uids("02_items.anm2");
    Selection selection = {};
    selection_references_set(selection, anm2, SelectionKind::FRAMES, {{0, LAYER, 0, 1}, {0, LAYER, 1, 0}});
    selection_focus_set(selection, anm2, {0, LAYER, 0, 1});

    selection_edit(anm2, selection, [&] { frame_insert(anm2, {0, LAYER, 0, -1}, 0); });
    selection_edit(anm2, selection, [&] { frames_reverse(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 1}}); });

    UidIndex index(anm2);
    CHECK(selection_focus_get(selection, index) == Reference{0, LAYER, 0, 0});
    CHECK(selection_references_get(selection, index, SelectionKind::FRAMES) ==
          std::set<Reference>{{0, LAYER, 0, 0}, {0, LAYER, 1, 0}});
}

TEST_CASE("a removed focus falls back to its nearest survivor")
{
    auto anm2 = fixture_load_with_uids("02_items.anm2");
    Selection selection = {};
    selection_focus_set(selection, anm2, {0, LAYER, 0, 1});
    selection_references_set(selection, anm2, SelectionKind::FRAMES, {{0, LAYER, 0, 1}});

    selection_edit(anm2, selection, [&] { frames_delete(anm2, {{0, LAYER, 0, 1}}); });
    CHECK(selection_focus_get(selection, UidIndex(anm2)) == Reference{0, LAYER, 0, 0});
    CHECK(selection.uids[SelectionKind::FRAMES].empty());

    selection_edit(anm2, selection, [&] { items_remove(anm2, 0, {{LAYER, {0}}}, {}); });
    CHECK(selection_focus_get(selection, UidIndex(anm2)) == Reference{0, ROOT});

    selection_focus_set(selection, anm2, {1, ROOT, -1, 0});
    selection_edit(anm2, selection, [&] { animations_remove(anm2, {1}, {}); });
    CHECK(selection_focus_get(selection, UidIndex(anm2)) == Reference{0, ROOT});
}

TEST_CASE("track and group selection follow regrouping")
{
    auto anm2 = fixture_load_with_uids("03a_groups_nested.anm2");
    const TrackGroup* group = nullptr;
    groups_each(anm2.animation_get(0)->layers, [&](const TrackGroup& candidate) { group = &candidate; });
    REQUIRE(group);
    auto groupId = group->id;
    Selection selection = {};
    selection_references_set(selection, anm2, SelectionKind::TRACKS, {{0, LAYER, 0, -1, LAYER, groupId}});
    selection_references_set(selection, anm2, SelectionKind::GROUPS, {{0, LAYER, groupId}});

    selection_edit(anm2, selection,
                   [&] { items_move(anm2, {.type = LAYER, .ids = {0}, .target = {false, LAYER, 2, -1}}); });

    UidIndex index(anm2);
    CHECK(selection_references_get(selection, index, SelectionKind::TRACKS) == std::set<Reference>{{0, LAYER, 0}});
    CHECK(selection_references_get(selection, index, SelectionKind::GROUPS) ==
          std::set<Reference>{{0, LAYER, groupId}});
}

TEST_CASE("content selection follows items, and regions follow the focused spritesheet")
{
    auto anm2 = fixture_load_with_uids("05_regions.anm2");
    Selection selection = {};
    auto& regions = item_get(anm2.content.spritesheets, 0)->regions;
    REQUIRE(regions.size() > 1);
    auto regionId = regions[1].id;

    selection_ids_set(selection, anm2, SelectionKind::REGIONS, {regionId});
    CHECK(selection_ids_get(selection, anm2, SelectionKind::REGIONS).empty());

    selection_focus_id_set(selection, anm2, SelectionKind::SPRITESHEETS, 0);
    selection_ids_set(selection, anm2, SelectionKind::REGIONS, {regionId});
    selection_focus_id_set(selection, anm2, SelectionKind::REGIONS, regionId);
    regions.erase(regions.begin());
    CHECK(selection_ids_get(selection, anm2, SelectionKind::REGIONS) == std::set<int>{regionId});
    CHECK(selection_focus_id_get(selection, anm2, SelectionKind::REGIONS) == regionId);

    std::erase_if(regions, [&](const Region& region) { return region.id == regionId; });
    CHECK(selection_ids_get(selection, anm2, SelectionKind::REGIONS).empty());
    CHECK(selection_focus_id_get(selection, anm2, SelectionKind::REGIONS) == -1);
}
