#include "common.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

Anm2 fixture_load_with_uids(const char* name)
{
  auto anm2 = anm2_load(file_load(CORPUS_DIR / name));
  anm2.uids_repair();
  return anm2;
}

Element& track_get(Anm2& anm2, Reference reference)
{
  reference.frameIndex = -1;
  auto track = anm2.element_get(reference);
  REQUIRE(track);
  return *track;
}

std::optional<Reference> reference_follow(Anm2& anm2, Reference reference, auto&& edit)
{
  auto handle = anm2.handle_get(reference);
  edit();
  anm2.uids_repair();
  return UidIndex(anm2).reference_get(handle, reference);
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

TEST_CASE("frame references follow inserts, deletes and copies")
{
  auto anm2 = fixture_load_with_uids("02_items.anm2");
  Reference frame{0, LAYER, 0, 1};

  auto inserted = reference_follow(anm2, frame,
                                   [&]
                                   {
                                     auto& track = track_get(anm2, frame);
                                     track.children.insert(track.children.begin(), element_make(ElementType::FRAME));
                                   });
  REQUIRE(inserted);
  CHECK(inserted->frameIndex == 2);
  frame = *inserted;

  auto copied = reference_follow(anm2, frame,
                                 [&]
                                 {
                                   auto& track = track_get(anm2, frame);
                                   track.children.push_back(track.children[frame.frameIndex]);
                                 });
  REQUIRE(copied);
  CHECK(copied->frameIndex == 2);

  CHECK_FALSE(reference_follow(
      anm2, frame, [&] { track_get(anm2, frame).children.erase(track_get(anm2, frame).children.begin() + 2); }));
}

TEST_CASE("references follow animation removal and reordering")
{
  auto anm2 = fixture_load_with_uids("02_items.anm2");
  Reference root{1, ROOT, -1, 0};
  auto animations = anm2.element_get(ElementType::ANIMATIONS);
  auto followed = reference_follow(anm2, root, [&] { animations->children.erase(animations->children.begin()); });
  REQUIRE(followed);
  CHECK(followed->animationIndex == 0);
  CHECK(followed->frameIndex == 0);

  Reference animationOnly{0};
  auto reordered = reference_follow(
      anm2, animationOnly,
      [&] { animations->children.insert(animations->children.begin(), element_make(ElementType::ANIMATION)); });
  REQUIRE(reordered);
  CHECK(reordered->animationIndex == 1);
}

TEST_CASE("track references follow regrouping")
{
  auto anm2 = fixture_load_with_uids("03a_groups_nested.anm2");
  auto animation = anm2.element_get(ElementType::ANIMATION, 0);
  auto layers = child_first_get(*animation, ElementType::LAYER_ANIMATIONS);
  auto group = child_first_get(*layers, ElementType::GROUP);
  REQUIRE(group);
  auto grouped = track_find(*layers, ElementType::LAYER_ANIMATION, 0);
  REQUIRE(grouped);
  REQUIRE(grouped->groupId == group->id);

  Reference withGroup{0, LAYER, 0, 0, LAYER, group->id};
  Reference withoutGroup{0, LAYER, 0, 0};
  auto handleWithGroup = anm2.handle_get(withGroup);
  auto handleWithoutGroup = anm2.handle_get(withoutGroup);
  grouped->groupId = -1;

  UidIndex index(anm2);
  auto followedWithGroup = index.reference_get(handleWithGroup, withGroup);
  auto followedWithoutGroup = index.reference_get(handleWithoutGroup, withoutGroup);
  REQUIRE(followedWithGroup);
  REQUIRE(followedWithoutGroup);
  CHECK(*followedWithGroup == Reference{0, LAYER, 0, 0});
  CHECK(*followedWithoutGroup == withoutGroup);
  CHECK(anm2.element_get(*followedWithGroup));

  Reference groupReference{0, LAYER, group->id};
  auto groupHandle = anm2.handle_get(groupReference, true);
  CHECK(groupHandle.item == group->uid);
  CHECK(index.reference_get(groupHandle, groupReference) == groupReference);
}
