#include "common.hpp"

#include "edit/edit.hpp"
#include "selection.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

Anm2 fixture_load_with_uids(const char* name)
{
  auto anm2 = anm2_load(file_load(CORPUS_DIR / name));
  anm2.uids_repair();
  return anm2;
}

// Applies an edit and follows the selection through it, the way a commit does.
void selection_edit(Anm2& anm2, Selection& selection, auto&& edit)
{
  auto before = selection_focus_get(selection, UidIndex(anm2));
  edit();
  anm2.uids_repair();
  selection_follow(selection, anm2, UidIndex(anm2), before);
}

TEST_CASE("uids are unique and non-zero after load")
{
  for (const auto& fixture : fixtures_get())
  {
    auto anm2 = anm2_load(file_load(fixture));
    std::set<std::uint64_t> uids{};
    bool isUnique{true};
    element_each(anm2.root,
                 [&](const Element& element) { isUnique &= element.uid != 0 && uids.insert(element.uid).second; });
    INFO("fixture: ", fixture.filename().string());
    CHECK(isUnique);
  }
}

TEST_CASE("selected frames and focus follow inserts and moves")
{
  auto anm2 = fixture_load_with_uids("02_items.anm2");
  Selection selection{};
  selection_references_set(selection, anm2, SelectionKind::FRAMES, {{0, LAYER, 0, 1}, {0, LAYER, 1, 0}});
  selection_focus_set(selection, anm2, {0, LAYER, 0, 1});

  selection_edit(anm2, selection, [&] { edit::frame_insert(anm2, {0, LAYER, 0, -1}, 0); });
  selection_edit(anm2, selection, [&] { edit::frames_reverse(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 1}}); });

  UidIndex index(anm2);
  CHECK(selection_focus_get(selection, index) == Reference{0, LAYER, 0, 0});
  CHECK(selection_references_get(selection, index, SelectionKind::FRAMES) ==
        std::set<Reference>{{0, LAYER, 0, 0}, {0, LAYER, 1, 0}});
}

TEST_CASE("a removed focus falls back to its nearest survivor")
{
  auto anm2 = fixture_load_with_uids("02_items.anm2");
  Selection selection{};
  selection_focus_set(selection, anm2, {0, LAYER, 0, 1});
  selection_references_set(selection, anm2, SelectionKind::FRAMES, {{0, LAYER, 0, 1}});

  selection_edit(anm2, selection, [&] { edit::frames_delete(anm2, {{0, LAYER, 0, 1}}); });
  CHECK(selection_focus_get(selection, UidIndex(anm2)) == Reference{0, LAYER, 0, 0});
  CHECK(selection.uids[SelectionKind::FRAMES].empty());

  selection_edit(anm2, selection, [&] { edit::items_remove(anm2, 0, {{LAYER, {0}}}, {}); });
  CHECK(selection_focus_get(selection, UidIndex(anm2)) == Reference{0, ROOT});

  selection_focus_set(selection, anm2, {1, ROOT, -1, 0});
  selection_edit(anm2, selection, [&] { edit::animations_remove(anm2, {1}, {}); });
  CHECK(selection_focus_get(selection, UidIndex(anm2)) == Reference{0, ROOT});
}

TEST_CASE("track and group selection follow regrouping")
{
  auto anm2 = fixture_load_with_uids("03a_groups_nested.anm2");
  auto group =
      child_first_get(*child_first_get(*anm2.element_get(ElementType::ANIMATION, 0), ElementType::LAYER_ANIMATIONS),
                      ElementType::GROUP);
  REQUIRE(group);
  Selection selection{};
  selection_references_set(selection, anm2, SelectionKind::TRACKS, {{0, LAYER, 0, -1, LAYER, group->id}});
  selection_references_set(selection, anm2, SelectionKind::GROUPS, {{0, LAYER, group->id}});

  selection_edit(anm2, selection,
                 [&] { edit::items_move(anm2, 0, LAYER, {0}, {}, {false, LAYER, 2, -1}, false, false); });

  UidIndex index(anm2);
  CHECK(selection_references_get(selection, index, SelectionKind::TRACKS) == std::set<Reference>{{0, LAYER, 0}});
  CHECK(selection_references_get(selection, index, SelectionKind::GROUPS) ==
        std::set<Reference>{{0, LAYER, group->id}});
}
