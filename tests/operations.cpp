#include "common.hpp"

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
