#include "common.hpp"

#include "edit/edit.hpp"
#include "util/pack.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

Anm2 fixture_load(const char* name) { return anm2_load(file_load(CORPUS_DIR / name)); }

void operation_check(const char* name, const Anm2& anm2)
{
  golden_check(GOLDEN_DIR / std::format("operation.{}.xml", name), anm2.to_string({.isExtendedFormat = true}));
}

Element& layer_track_get(Anm2& anm2, int animationIndex, int layerId)
{
  auto track = animation_item_get(*anm2.element_get(ElementType::ANIMATION, animationIndex), ItemType::LAYER, layerId);
  REQUIRE(track);
  return *track;
}

int groups_named_count(const Element& element, std::string_view name)
{
  int count{};
  element_each(element, [&](const Element& child) { count += child.type == ElementType::GROUP && child.name == name; });
  return count;
}

TEST_CASE("animations_merge keeps one group per name")
{
  for (auto [name, type] : MERGES)
  {
    INFO("merge: ", std::string(name));
    auto anm2 = fixture_load("03a_groups_nested.anm2");
    std::set<int> sources{0, 1};
    anm2.animations_merge(0, sources, type, false);
    CHECK(groups_named_count(*anm2.element_get(ElementType::ANIMATION, 0), "Main") == 1);
  }
}

TEST_CASE("file_merge presets")
{
  constexpr std::pair<const char*, FileMergePreset> PRESETS_FILE_MERGE[] = {
      {"file_merge_by_name", FILE_MERGE_PRESET_MERGE_BY_NAME},
      {"file_merge_append", FILE_MERGE_PRESET_APPEND_AS_NEW},
      {"file_merge_replace", FILE_MERGE_PRESET_REPLACE_MATCHING}};
  for (auto [name, preset] : PRESETS_FILE_MERGE)
  {
    auto anm2 = fixture_load("02_items.anm2");
    CHECK(anm2.file_merge(CORPUS_DIR / "05_regions.anm2", CORPUS_DIR, preset));
    operation_check(name, anm2);
  }
}

TEST_CASE("item_add")
{
  auto anm2 = fixture_load("02_items.anm2");
  auto layer = element_make(ElementType::LAYER_ELEMENT);
  layer.name = "added";
  layer.spritesheetId = 1;
  CHECK(anm2.item_add(ItemType::LAYER, 0, layer, 1, types::destination::ALL) == 2);
  auto null = element_make(ElementType::NULL_ELEMENT);
  null.name = "thisOnly";
  CHECK(anm2.item_add(ItemType::NULL_, 1, null, -1, types::destination::THIS) == 2);
  operation_check("item_add", anm2);
}

TEST_CASE("regions_generate and regions_scan")
{
  auto anm2 = fixture_load("02_items.anm2");
  CHECK(anm2.regions_generate({0}, {}, "Region {}", RegionFrameMapping::SET));
  CHECK_FALSE(anm2.regions_generate({0}, {}, "Region {}", RegionFrameMapping::SET));
  operation_check("regions_generate", anm2);

  auto scanned = fixture_load("05_regions.anm2");
  scanned.regions_scan();
  operation_check("regions_scan", scanned);
}

TEST_CASE("frames_change")
{
  constexpr std::pair<const char*, ChangeType> CHANGES[] = {{"adjust", ChangeType::ADJUST},
                                                            {"add", ChangeType::ADD},
                                                            {"subtract", ChangeType::SUBTRACT},
                                                            {"multiply", ChangeType::MULTIPLY},
                                                            {"divide", ChangeType::DIVIDE}};
  for (auto [name, type] : CHANGES)
  {
    auto anm2 = fixture_load("02_items.anm2");
    FrameChange change{.cropX = 2.0f, .positionY = 4.0f, .scaleX = 50.0f, .rotation = 90.0f, .tintR = 0.5f};
    change.duration = 3;
    change.interpolation = Interpolation::EASE_IN;
    change.isFlipX = true;
    frames_change(layer_track_get(anm2, 0, 0), change, ItemType::LAYER, type, {0, 1});
    operation_check(std::format("frames_change_{}", name).c_str(), anm2);
  }
}

TEST_CASE("frame_bake and frame_mix")
{
  auto anm2 = fixture_load("07_special_interpolation.anm2");
  frame_bake(layer_track_get(anm2, 0, 0), 0, 2, true, false);
  frame_bake(layer_track_get(anm2, 0, 0), 3, 1, false, true);
  operation_check("frame_bake", anm2);

  auto from = element_make(ElementType::FRAME);
  auto to = element_make(ElementType::FRAME);
  to.position = {10.0f, -20.0f};
  to.shear = {30.0f, 0.0f};
  to.rotation = 90.0f;
  to.tint = {0.0f, 1.0f, 1.0f, 0.5f};
  frame_mix(from, to, 0.5f);
  CHECK(from.position == glm::vec2(5.0f, -10.0f));
  CHECK(from.shear == glm::vec2(15.0f, 0.0f));
  CHECK(from.rotation == doctest::Approx(45.0f));
  CHECK(from.tint.a == doctest::Approx(0.75f));
}

TEST_CASE("rects_pack fits every rect without overlap")
{
  std::vector<glm::ivec2> sizes{{16, 16}, {32, 8}, {8, 32}, {5, 7}, {64, 1}, {1, 1}};
  glm::ivec2 size{};
  std::vector<glm::ivec2> positions{};
  REQUIRE(util::pack::rects_pack(sizes, size, positions));
  REQUIRE(positions.size() == sizes.size());
  for (std::size_t i = 0; i < sizes.size(); ++i)
  {
    CHECK(glm::all(glm::greaterThanEqual(positions[i], glm::ivec2(0))));
    CHECK(glm::all(glm::lessThanEqual(positions[i] + sizes[i], size)));
    for (std::size_t j = i + 1; j < sizes.size(); ++j)
    {
      auto isSeparate = glm::any(glm::lessThanEqual(positions[i] + sizes[i], positions[j])) ||
                        glm::any(glm::lessThanEqual(positions[j] + sizes[j], positions[i]));
      CHECK(isSeparate);
    }
  }
}

// Each edit runs on a fixture; its result document is a golden, and every returned uid must resolve.
TEST_CASE("edit layer operations")
{
  struct Case
  {
    const char* name;
    const char* fixture;
    std::function<edit::Uids(Anm2&)> operation;
    std::size_t uidCount;
  };

  const Reference head{0, LAYER, 0};
  const Case CASES[] = {
      {"frame_insert", "02_items.anm2", [&](Anm2& anm2) { return edit::frame_insert(anm2, {0, LAYER, 0, 0}, 0); }, 1},
      {"trigger_insert", "02_items.anm2", [&](Anm2& anm2) { return edit::frame_insert(anm2, {0, TRIGGER, -1, 0}, 2); },
       1},
      {"frames_delete", "02_items.anm2",
       [&](Anm2& anm2) { return edit::frames_delete(anm2, {{0, LAYER, 0, 0}, {0, ROOT, -1, 1}}); }, 0},
      {"frames_duplicate", "02_items.anm2", [&](Anm2& anm2)
       { return edit::frames_duplicate(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 1}}, {0, LAYER, 0, 1}); }, 2},
      {"frames_reverse", "07_special_interpolation.anm2",
       [&](Anm2& anm2) { return edit::frames_reverse(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 2}}); }, 0},
      {"frames_bake", "07_special_interpolation.anm2",
       [&](Anm2& anm2) { return edit::frames_bake(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 1}}, 2, true, false); }, 5},
      {"frame_split", "02_items.anm2", [&](Anm2& anm2) { return edit::frame_split(anm2, {0, LAYER, 1, 0}, 2); }, 1},
      {"frames_move", "02_items.anm2",
       [&](Anm2& anm2) { return edit::frames_move(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 1, 0}}, head, 2); }, 2},
      {"frames_paste", "02_items.anm2",
       [&](Anm2& anm2)
       {
         auto frame = anm2.element_get(Reference{0, LAYER, 1, 0});
         auto text = element_to_string(*frame, ElementType::LAYER_ANIMATION) +
                     element_to_string(*frame, ElementType::LAYER_ANIMATION);
         return edit::frames_paste(anm2, {0, LAYER, 0, 0}, {}, text, 0, nullptr);
       },
       2},
      {"root_bake_into", "04_group_root_transform.anm2",
       [&](Anm2& anm2)
       {
         return edit::root_bake_into(anm2, {{0, ROOT, -1, 0}}, {{0, LAYER, 2}},
                                     {.isRoundScale = true, .isMatchRootInterpolation = true});
       },
       1},
      {"items_group", "03b_groups_legacy_flat.anm2",
       [&](Anm2& anm2) { return edit::items_group(anm2, 0, LAYER, {2}, "New"); }, 1},
      {"items_move", "03b_groups_legacy_flat.anm2",
       [&](Anm2& anm2) { return edit::items_move(anm2, 0, LAYER, {2}, {}, {true, LAYER, 0, -1}, false, true); }, 0},
      {"items_remove", "03b_groups_legacy_flat.anm2",
       [&](Anm2& anm2) { return edit::items_remove(anm2, 0, {{LAYER, {2}}}, {{NULL_, {3}}}); }, 0},
      {"animation_add", "03a_groups_nested.anm2",
       [&](Anm2& anm2) { return edit::animation_add(anm2, 1, 0, 0, "Added"); }, 1},
      {"animations_remove", "03a_groups_nested.anm2",
       [&](Anm2& anm2) { return edit::animations_remove(anm2, {1}, {0}); }, 0},
      {"animations_move", "02_items.anm2", [&](Anm2& anm2) { return edit::animations_move(anm2, {0}, {}, 2, -1); }, 1},
      {"animations_group", "02_items.anm2", [&](Anm2& anm2) { return edit::animations_group(anm2, {0, 1}, "Group"); },
       0},
      {"animations_paste", "02_items.anm2",
       [&](Anm2& anm2)
       {
         std::set<int> groupIds{};
         auto text = element_to_string(*anm2.element_get(ElementType::ANIMATION, 1));
         return edit::animations_paste(anm2, text, 0, -1, groupIds, nullptr);
       },
       1},
      {"regions_remove_unused", "05_regions.anm2", [&](Anm2& anm2) { return edit::regions_remove_unused(anm2, 0); }, 0},
      {"regions_paste", "05_regions.anm2",
       [&](Anm2& anm2)
       {
         auto region = child_id_get(*anm2.element_get(ElementType::SPRITESHEET, 0), ElementType::REGION, 1);
         return edit::regions_paste(anm2, 0, element_to_string(*region), 1, nullptr);
       },
       0},
      {"animation_grid_generate", "05_regions.anm2",
       [&](Anm2& anm2)
       {
         return edit::animation_grid_generate(anm2, {{0, LAYER, 0}},
                                              {{0, 0}, {8, 8}, {4, 4}, 2, 3, 2, true, "Grid {}"});
       },
       3},
      {"frames_change_apply", "02_items.anm2",
       [&](Anm2& anm2)
       {
         FrameChange change{.positionX = 3.0f, .rotation = 10.0f};
         return edit::frames_change_apply(
             anm2, {.references = {{0, LAYER, 0, 1}, {0, NULL_, 0}}, .animations = {1}, .isRoot = true}, change,
             ChangeType::ADD);
       },
       0},
      {"animations_merge", "03a_groups_nested.anm2",
       [&](Anm2& anm2) { return edit::animations_merge(anm2, 0, {1}, types::merge::APPEND, true, {0}); }, 1},
  };

  for (const auto& entry : CASES)
  {
    INFO("operation: ", std::string(entry.name));
    auto anm2 = fixture_load(entry.fixture);
    anm2.uids_repair();
    auto uids = entry.operation(anm2);
    CHECK(uids.size() == entry.uidCount);
    anm2.uids_repair();
    UidIndex index(anm2);
    for (auto uid : uids)
      CHECK(index.reference_get(uid));
    operation_check(entry.name, anm2);
  }
}
