#include "common.hpp"

#include "model/frames.hpp"
#include "util/pack.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

model::Model fixture_load(const char* name) { return model_load(file_load(CORPUS_DIR / name)); }

void operation_check(const char* name, const model::Model& model)
{
  golden_check(GOLDEN_DIR / std::format("operation.{}.xml", name),
               model::model_to_string(model, {.isExtendedFormat = true}));
}

model::Track& layer_track_get(model::Model& model, int animationIndex, int layerId)
{
  auto track = model.track_edit({animationIndex, LAYER, layerId});
  REQUIRE(track);
  return *track;
}

int groups_named_count(const model::Animation& animation, std::string_view name)
{
  int count{};
  for (const auto* entries : {&animation.layers, &animation.nulls})
    model::groups_each(*entries, [&](const model::TrackGroup& group) { count += group.name == name; });
  return count;
}

TEST_CASE("animations_merge keeps one group per name")
{
  for (auto [name, type] : MERGES)
  {
    INFO("merge: ", std::string(name));
    auto model = fixture_load("03a_groups_nested.anm2");
    edit::animations_merge(model, 0, {0, 1}, type, false, {});
    CHECK(groups_named_count(*model.animation_get(0), "Main") == 1);
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
    auto model = fixture_load("02_items.anm2");
    CHECK(edit::file_merge(model, CORPUS_DIR / "05_regions.anm2", CORPUS_DIR, preset));
    operation_check(name, model);
  }
}

TEST_CASE("item_add")
{
  auto model = fixture_load("02_items.anm2");
  CHECK(edit::item_add(model, ItemType::LAYER, 0, -1, "added", 1, false, 1, types::destination::ALL) == 2);
  CHECK(edit::item_add(model, ItemType::NULL_, 1, -1, "thisOnly", 0, false, -1, types::destination::THIS) == 2);
  operation_check("item_add", model);
}

TEST_CASE("regions_generate and regions_scan")
{
  auto model = fixture_load("02_items.anm2");
  edit::regions_generate(model, {0}, {}, "Region {}", RegionFrameMapping::SET);
  auto generated = model;
  edit::regions_generate(model, {0}, {}, "Region {}", RegionFrameMapping::SET);
  CHECK(model::is_model_equal(model, generated));
  operation_check("regions_generate", model);

  auto scanned = fixture_load("05_regions.anm2");
  edit::regions_scan(scanned);
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
    auto model = fixture_load("02_items.anm2");
    FrameChange change{.cropX = 2.0f, .positionY = 4.0f, .scaleX = 50.0f, .rotation = 90.0f, .tintR = 0.5f};
    change.duration = 3;
    change.interpolation = Interpolation::EASE_IN;
    change.isFlipX = true;
    model::frames_change(layer_track_get(model, 0, 0), change, ItemType::LAYER, type, {0, 1});
    operation_check(std::format("frames_change_{}", name).c_str(), model);
  }
}

TEST_CASE("frame_bake and frame_mix")
{
  auto model = fixture_load("07_special_interpolation.anm2");
  model::frame_bake(layer_track_get(model, 0, 0), 0, 2, true, false);
  model::frame_bake(layer_track_get(model, 0, 0), 3, 1, false, true);
  operation_check("frame_bake", model);

  model::Frame from{};
  model::Frame to{};
  to.position = {10.0f, -20.0f};
  to.shear = {30.0f, 0.0f};
  to.rotation = 90.0f;
  to.tint = {0.0f, 1.0f, 1.0f, 0.5f};
  model::frame_mix(from, to, 0.5f);
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
    std::function<edit::Uids(model::Model&)> operation;
    std::size_t uidCount;
  };

  const Reference head{0, LAYER, 0};
  const Case CASES[] = {
      {"frame_insert", "02_items.anm2",
       [&](model::Model& anm2) { return edit::frame_insert(anm2, {0, LAYER, 0, 0}, 0); }, 1},
      {"trigger_insert", "02_items.anm2",
       [&](model::Model& anm2) { return edit::frame_insert(anm2, {0, TRIGGER, -1, 0}, 2); }, 1},
      {"frames_delete", "02_items.anm2",
       [&](model::Model& anm2) { return edit::frames_delete(anm2, {{0, LAYER, 0, 0}, {0, ROOT, -1, 1}}); }, 0},
      {"frames_duplicate", "02_items.anm2", [&](model::Model& anm2)
       { return edit::frames_duplicate(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 1}}, {0, LAYER, 0, 1}); }, 2},
      {"frames_reverse", "07_special_interpolation.anm2",
       [&](model::Model& anm2) { return edit::frames_reverse(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 2}}); }, 0},
      {"frames_bake", "07_special_interpolation.anm2", [&](model::Model& anm2)
       { return edit::frames_bake(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 1}}, 2, true, false); }, 5},
      {"frame_split", "02_items.anm2", [&](model::Model& anm2) { return edit::frame_split(anm2, {0, LAYER, 1, 0}, 2); },
       1},
      {"frames_move", "02_items.anm2",
       [&](model::Model& anm2) { return edit::frames_move(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 1, 0}}, head, 2); }, 2},
      {"frames_paste", "02_items.anm2",
       [&](model::Model& anm2)
       {
         auto frame = anm2.frame_get(Reference{0, LAYER, 1, 0});
         auto text = model::frame_to_string(*frame, ItemType::LAYER) + model::frame_to_string(*frame, ItemType::LAYER);
         return edit::frames_paste(anm2, {0, LAYER, 0, 0}, {}, text, 0, nullptr);
       },
       2},
      {"root_bake_into", "04_group_root_transform.anm2",
       [&](model::Model& anm2)
       {
         return edit::root_bake_into(anm2, {{0, ROOT, -1, 0}}, {{0, LAYER, 2}},
                                     {.isRoundScale = true, .isMatchRootInterpolation = true});
       },
       1},
      {"items_group", "03b_groups_legacy_flat.anm2",
       [&](model::Model& anm2) { return edit::items_group(anm2, 0, LAYER, {2}, "New"); }, 1},
      {"items_move", "03b_groups_legacy_flat.anm2", [&](model::Model& anm2)
       { return edit::items_move(anm2, 0, LAYER, {2}, {}, {true, LAYER, 0, -1}, false, true); }, 0},
      {"items_remove", "03b_groups_legacy_flat.anm2",
       [&](model::Model& anm2) { return edit::items_remove(anm2, 0, {{LAYER, {2}}}, {{NULL_, {3}}}); }, 0},
      {"animation_add", "03a_groups_nested.anm2",
       [&](model::Model& anm2) { return edit::animation_add(anm2, 1, 0, 0, "Added"); }, 1},
      {"animations_remove", "03a_groups_nested.anm2",
       [&](model::Model& anm2) { return edit::animations_remove(anm2, {1}, {0}); }, 0},
      {"animations_move", "02_items.anm2", [&](model::Model& anm2)
       { return edit::animations_move(anm2, {0}, {}, {.animationIndex = 1, .isAfter = true}); }, 1},
      {"animations_group", "02_items.anm2",
       [&](model::Model& anm2) { return edit::animations_group(anm2, {0, 1}, "Group"); }, 0},
      {"animations_paste", "02_items.anm2",
       [&](model::Model& anm2)
       {
         std::set<int> groupIds{};
         auto text = model::animation_to_string(*anm2.animation_get(1));
         return edit::animations_paste(anm2, text, 0, -1, groupIds, nullptr);
       },
       1},
      {"regions_remove_unused", "05_regions.anm2",
       [&](model::Model& anm2) { return edit::regions_remove_unused(anm2, 0); }, 0},
      {"regions_paste", "05_regions.anm2",
       [&](model::Model& anm2)
       {
         auto region = model::item_get(model::item_get(anm2.content.spritesheets, 0)->regions, 1);
         return edit::regions_paste(anm2, 0, model::item_to_string(*region), 1, nullptr);
       },
       0},
      {"animation_grid_generate", "05_regions.anm2",
       [&](model::Model& anm2)
       {
         return edit::animation_grid_generate(anm2, {{0, LAYER, 0}},
                                              {{0, 0}, {8, 8}, {4, 4}, 2, 3, 2, true, "Grid {}"});
       },
       3},
      {"frames_change_apply", "02_items.anm2",
       [&](model::Model& anm2)
       {
         FrameChange change{.positionX = 3.0f, .rotation = 10.0f};
         return edit::frames_change_apply(
             anm2, {.references = {{0, LAYER, 0, 1}, {0, NULL_, 0}}, .animations = {1}, .isRoot = true}, change,
             ChangeType::ADD);
       },
       0},
      {"animations_merge", "03a_groups_nested.anm2",
       [&](model::Model& anm2) { return edit::animations_merge(anm2, 0, {1}, types::merge::APPEND, true, {0}); }, 1},
  };

  for (const auto& entry : CASES)
  {
    INFO("operation: ", std::string(entry.name));
    auto anm2 = fixture_load(entry.fixture);
    auto uids = entry.operation(anm2);
    CHECK(uids.size() == entry.uidCount);
    anm2.uids_repair();
    model::UidIndex index(anm2);
    for (auto uid : uids)
      CHECK(index.reference_get(uid));
    operation_check(entry.name, anm2);
  }
}

TEST_CASE("spritesheet pixel edits")
{
  auto image_make = [](glm::ivec2 size, std::vector<glm::ivec2> opaque = {})
  {
    std::vector<uint8_t> pixels((std::size_t)size.x * size.y * resource::image::CHANNELS, 0);
    for (auto point : opaque)
      std::fill_n(pixels.begin() + ((std::size_t)point.y * size.x + point.x) * resource::image::CHANNELS,
                  resource::image::CHANNELS, 255);
    return resource::Image(pixels.data(), size);
  };
  auto sheet = image_make({64, 64}, {{4, 6}});
  auto other = image_make({16, 16});

  SUBCASE("regions_trim shrinks to content and keeps the custom pivot in place")
  {
    auto model = fixture_load("05_regions.anm2");
    CHECK(edit::regions_trim(model, 0, {0, 1}, sheet));
    auto region = model::item_get(model.content.spritesheets[0].regions, 0);
    CHECK(region->crop == glm::vec2(4, 6));
    CHECK(region->size == glm::vec2(1, 1));
    CHECK(region->pivot == glm::vec2(-1, -1));
  }

  SUBCASE("spritesheet_pack returns an image holding every region")
  {
    auto model = fixture_load("05_regions.anm2");
    auto packed = edit::spritesheet_pack(model, 0, sheet, 1);
    REQUIRE(packed);
    CHECK(packed->is_valid());
    for (auto& region : model.content.spritesheets[0].regions)
      CHECK(glm::all(glm::lessThanEqual(glm::ivec2(region.crop + region.size), packed->size)));
  }

  SUBCASE("spritesheets_merge appends images and moves layers onto the base sheet")
  {
    auto model = fixture_load("05_regions.anm2");
    auto image_get = [&](int id) { return id == 0 ? &sheet : &other; };
    auto merged = edit::spritesheets_merge(model, {0, 1}, image_get, true, true, false, Origin::TOP_LEFT);
    REQUIRE(merged);
    CHECK(merged->size == glm::ivec2(80, 64));
    CHECK(model.content.spritesheets.size() == 1);
    CHECK(model::item_get(model.content.layers, 1)->spritesheetId == 0);
    operation_check("spritesheets_merge", model);
  }
}
