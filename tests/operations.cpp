#include "common.h"

#include "model/frames.h"
#include "playback.h"
#include "pack.h"

static Model fixture_load(const char* name)
{
    return model_load(file_load(CORPUS_DIR / name));
}

static void operation_check(const char* name, const Model& model)
{
    golden_check(GOLDEN_DIR / std::format("operation.{}.xml", name), model_serialize(model, SAVE_EDITOR));
}

static Track& layer_track_get(Model& model, int animationIndex, int layerId)
{
    auto track = model.track_edit({animationIndex, LAYER, layerId});
    REQUIRE(track);
    return *track;
}

static int groups_named_count(const Animation& animation, std::string_view name)
{
    int count = 0;
    for (const auto* entries : {&animation.layers, &animation.nulls})
        groups_each(*entries, [&](const TrackGroup& group) { count += group.name == name; });
    return count;
}

TEST_CASE("animations_merge keeps one group per name")
{
    for (auto [name, type] : MERGES)
    {
        INFO("merge: ", std::string(name));
        auto model = fixture_load("03a_groups_nested.anm2");
        animations_merge(model, 0, {0, 1}, type, SOURCES_KEPT, {});
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
        CHECK(file_merge(model, CORPUS_DIR / "05_regions.anm2", CORPUS_DIR, preset));
        operation_check(name, model);
    }
}

TEST_CASE("item_add")
{
    auto model = fixture_load("02_items.anm2");
    CHECK(item_add(model, ItemType::LAYER, 0, -1, {.name = "added", .spritesheetId = 1}, {}, 1, DESTINATION_ALL) == 2);
    CHECK(item_add(model, ItemType::NULL_, 1, -1, {}, {.name = "thisOnly"}, -1, DESTINATION_THIS) == 2);
    operation_check("item_add", model);
}

TEST_CASE("regions_generate and regions_scan")
{
    auto model = fixture_load("02_items.anm2");
    regions_generate(model, {0}, {}, "Region {}", RegionFrameMapping::SET);
    auto generated = model;
    regions_generate(model, {0}, {}, "Region {}", RegionFrameMapping::SET);
    CHECK(is_model_equal(model, generated));
    operation_check("regions_generate", model);

    auto scanned = fixture_load("05_regions.anm2");
    regions_scan(scanned);
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
        FrameChange change = {.cropX = 2.0f, .positionY = 4.0f, .scaleX = 50.0f, .rotation = 90.0f, .tintR = 0.5f};
        change.duration = 3;
        change.interpolation = Interpolation::EASE_IN;
        change.isFlipX = true;
        frames_change(layer_track_get(model, 0, 0), change, ItemType::LAYER, type, {0, 1});
        operation_check(std::format("frames_change_{}", name).c_str(), model);
    }
}

TEST_CASE("frame_bake and frame_mix")
{
    auto model = fixture_load("07_special_interpolation.anm2");
    frame_bake(layer_track_get(model, 0, 0), 0, 2, {.isRoundScale = true});
    frame_bake(layer_track_get(model, 0, 0), 3, 1, {.isRoundRotation = true});
    operation_check("frame_bake", model);

    Frame from = {};
    Frame to = {};
    to.position = {10.0f, -20.0f};
    to.skew = {30.0f, 0.0f};
    to.rotation = 90.0f;
    to.tint = {0.0f, 1.0f, 1.0f, 0.5f};
    frame_mix(from, to, 0.5f);
    CHECK(from.position == Vec2(5.0f, -10.0f));
    CHECK(from.skew == Vec2(15.0f, 0.0f));
    CHECK(from.rotation == doctest::Approx(45.0f));
    CHECK(from.tint.a == doctest::Approx(0.75f));
}

TEST_CASE("rects_pack fits every rect without overlap")
{
    std::vector<IVec2> sizes{{16, 16}, {32, 8}, {8, 32}, {5, 7}, {64, 1}, {1, 1}};
    IVec2 size{};
    std::vector<IVec2> positions{};
    REQUIRE(rects_pack(sizes, size, positions));
    REQUIRE(positions.size() == sizes.size());
    for (std::size_t i = 0; i < sizes.size(); ++i)
    {
        CHECK(is_all_less_equal(IVec2(0), positions[i]));
        CHECK(is_all_less_equal(positions[i] + sizes[i], size));
        for (std::size_t j = i + 1; j < sizes.size(); ++j)
        {
            auto isSeparate = is_any_less_equal(positions[i] + sizes[i], positions[j]) ||
                              is_any_less_equal(positions[j] + sizes[j], positions[i]);
            CHECK(isSeparate);
        }
    }
}

// Each edit runs on a fixture. Its result document is a golden, and every returned uid must resolve.
TEST_CASE("edit layer operations")
{
    struct Case
    {
        const char* name;
        const char* fixture;
        std::function<Uids(Model&)> operation;
        std::size_t uidCount;
    };

    const Reference head = {0, LAYER, 0};
    const Case CASES[] = {
        {"frame_insert", "02_items.anm2", [&](Model& anm2) { return frame_insert(anm2, {0, LAYER, 0, 0}, 0); }, 1},
        {"trigger_insert", "02_items.anm2", [&](Model& anm2) { return frame_insert(anm2, {0, TRIGGER, -1, 0}, 2); }, 1},
        {"frames_delete", "02_items.anm2",
         [&](Model& anm2) {
             frames_delete(anm2, {{0, LAYER, 0, 0}, {0, ROOT, -1, 1}});
             return Uids{};
         },
         0},
        {"frames_duplicate", "02_items.anm2",
         [&](Model& anm2) { return frames_duplicate(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 1}}, {0, LAYER, 0, 1}); },
         2},
        {"frames_reverse", "07_special_interpolation.anm2",
         [&](Model& anm2) {
             frames_reverse(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 2}});
             return Uids{};
         },
         0},
        {"frames_bake", "07_special_interpolation.anm2",
         [&](Model& anm2) {
             return frames_bake(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 0, 1}}, 2, {.isRoundScale = true});
         },
         5},
        {"frame_split", "02_items.anm2", [&](Model& anm2) { return frame_split(anm2, {0, LAYER, 1, 0}, 2); }, 1},
        {"frames_move", "02_items.anm2",
         [&](Model& anm2) { return frames_move(anm2, {{0, LAYER, 0, 0}, {0, LAYER, 1, 0}}, head, 2); }, 2},
        {"frames_paste", "02_items.anm2",
         [&](Model& anm2) {
             auto frame = anm2.frame_get(Reference{0, LAYER, 1, 0});
             auto text = frame_to_string(*frame, ItemType::LAYER) + frame_to_string(*frame, ItemType::LAYER);
             return frames_paste(anm2, {0, LAYER, 0, 0}, {}, text, 0, nullptr);
         },
         2},
        {"root_bake_into", "04_group_root_transform.anm2",
         [&](Model& anm2) {
             return root_bake_into(anm2, {{0, ROOT, -1, 0}}, {{0, LAYER, 2}},
                                   {.isRoundScale = true, .isMatchRootInterpolation = true});
         },
         1},
        {"items_group", "03b_groups_legacy_flat.anm2",
         [&](Model& anm2) { return items_group(anm2, 0, LAYER, {2}, "New"); }, 1},
        {"items_move", "03b_groups_legacy_flat.anm2",
         [&](Model& anm2) {
             items_move(anm2, 0, LAYER, {2}, {}, {true, LAYER, 0, -1}, DROP_INSIDE);
             return Uids{};
         },
         0},
        {"items_remove", "03b_groups_legacy_flat.anm2",
         [&](Model& anm2) {
             items_remove(anm2, 0, {{LAYER, {2}}}, {{NULL_, {3}}});
             return Uids{};
         },
         0},
        {"animation_add", "03a_groups_nested.anm2", [&](Model& anm2) { return animation_add(anm2, 1, 0, 0, "Added"); },
         1},
        {"animations_remove", "03a_groups_nested.anm2",
         [&](Model& anm2) {
             animations_remove(anm2, {1}, {0});
             return Uids{};
         },
         0},
        {"animations_move", "02_items.anm2",
         [&](Model& anm2) { return animations_move(anm2, {0}, {}, {.animationIndex = 1, .isAfter = true}); }, 1},
        {"animations_group", "02_items.anm2",
         [&](Model& anm2) {
             animations_group(anm2, {0, 1}, "Group");
             return Uids{};
         },
         0},
        {"animations_paste", "02_items.anm2",
         [&](Model& anm2) {
             std::set<int> groupIds;
             auto text = animation_to_string(*anm2.animation_get(1));
             return animations_paste(anm2, text, 0, -1, groupIds, nullptr);
         },
         1},
        {"regions_paste", "05_regions.anm2",
         [&](Model& anm2) {
             auto region = item_get(item_get(anm2.content.spritesheets, 0)->regions, 1);
             regions_paste(anm2, 0, item_to_string(*region), 1, nullptr);
             return Uids{};
         },
         0},
        {"animation_grid_generate", "05_regions.anm2",
         [&](Model& anm2) {
             return animation_grid_generate(anm2, {{0, LAYER, 0}}, {{0, 0}, {8, 8}, {4, 4}, 2, 3, 2, true, "Grid {}"});
         },
         3},
        {"frames_change_apply", "02_items.anm2",
         [&](Model& anm2) {
             FrameChange change = {.positionX = 3.0f, .rotation = 10.0f};
             return frames_change_apply(
                 anm2, {.references = {{0, LAYER, 0, 1}, {0, NULL_, 0}}, .animations = {1}, .isRoot = true}, change,
                 ChangeType::ADD);
         },
         0},
        {"animations_merge", "03a_groups_nested.anm2",
         [&](Model& anm2) { return animations_merge(anm2, 0, {1}, MERGE_APPEND, SOURCES_DELETED, {0}); }, 1},
    };

    for (auto& entry : CASES)
    {
        INFO("operation: ", std::string(entry.name));
        auto anm2 = fixture_load(entry.fixture);
        auto uids = entry.operation(anm2);
        CHECK(uids.size() == entry.uidCount);
        anm2.uids_repair();
        UidIndex index(anm2);
        for (auto uid : uids)
            CHECK(index.reference_get(uid));
        operation_check(entry.name, anm2);
    }
}

TEST_CASE("spritesheet pixel edits")
{
    auto image_make = [](IVec2 size, std::vector<IVec2> opaque = {}) {
        std::vector<uint8_t> pixels((std::size_t)size.x * size.y * IMAGE_CHANNELS, 0);
        for (auto point : opaque)
            std::fill_n(pixels.begin() + ((std::size_t)point.y * size.x + point.x) * IMAGE_CHANNELS, IMAGE_CHANNELS,
                        255);
        return Image(pixels.data(), size);
    };
    auto sheet = image_make({64, 64}, {{4, 6}});
    auto other = image_make({16, 16});

    SUBCASE("regions_trim shrinks to content and keeps the custom pivot in place")
    {
        auto model = fixture_load("05_regions.anm2");
        CHECK(regions_trim(model, 0, {0, 1}, sheet));
        auto region = item_get(model.content.spritesheets[0].regions, 0);
        CHECK(region->crop == Vec2(4, 6));
        CHECK(region->size == Vec2(1, 1));
        CHECK(region->pivot == Vec2(-1, -1));
    }

    SUBCASE("spritesheet_pack returns an image holding every region")
    {
        auto model = fixture_load("05_regions.anm2");
        auto packed = spritesheet_pack(model, 0, sheet, 1);
        REQUIRE(packed);
        CHECK(packed->is_valid());
        for (auto& region : model.content.spritesheets[0].regions)
            CHECK(is_all_less_equal(IVec2(region.crop + region.size), packed->size));
    }

    SUBCASE("spritesheets_merge appends images and moves layers onto the base sheet")
    {
        auto model = fixture_load("05_regions.anm2");
        auto image_get = [&](int id) {
            return id == 0 ? &sheet : &other;
        };
        auto merged = spritesheets_merge(model, {0, 1}, image_get, {true, true, false, Origin::TOP_LEFT});
        REQUIRE(merged);
        CHECK(merged->size == IVec2(80, 64));
        CHECK(model.content.spritesheets.size() == 1);
        CHECK(item_get(model.content.layers, 1)->spritesheetId == 0);
        operation_check("spritesheets_merge", model);
    }
}

TEST_CASE("markers stay in the editor's copy and bound playback")
{
    auto model = fixture_load("01_minimal.anm2");
    auto animation = model.animation_edit(0);
    REQUIRE(animation);
    animation->frameNum = 10;
    animation->startMarker = 2;
    animation->endMarker = 5;
    CHECK(animation_play_range_get(*animation) == IVec2(2, 6));

    auto text = model_to_string(model);
    CHECK(game_document_get(text).find("Marker") == std::string::npos);
    auto reloaded = model_load(text);
    CHECK(reloaded.animation_get(0)->startMarker == 2);
    CHECK(reloaded.animation_get(0)->endMarker == 5);

    Playback playback = {};
    playback.isPlaying = true;
    playback.tick(10, 2, 6, PLAYBACK_LOOP, 0.1f);
    CHECK(playback.time == doctest::Approx(3.0f));
    playback.tick(10, 2, 6, PLAYBACK_LOOP, 0.5f);
    CHECK(playback.time == doctest::Approx(4.0f));
}

TEST_CASE("isaac_issues_get counts what the game's copy changes")
{
    CHECK(isaac_issues_get(fixture_load("01_minimal.anm2")) == IsaacIssues{});
    CHECK(isaac_issues_get(fixture_load("07_special_interpolation.anm2")).easedFrames == 5);
    CHECK(isaac_issues_get(fixture_load("04_group_root_transform.anm2")).groupTransforms > 0);
    auto shaders = isaac_issues_get(fixture_load("06_shaders.anm2"));
    CHECK(shaders.shaderFrames > 0);
    CHECK(shaders.skewFrames > 0);
    CHECK(isaac_issues_get(fixture_load("02_items.anm2")).triggerSounds > 0);
}

TEST_CASE("frames_property_set copies one property to the frames that have it")
{
    auto model = fixture_load("02_items.anm2");
    Frame source = {.crop = {7, 8}, .position = {3, 4}};
    Reference layer = {0, LAYER, 0, 0};
    Reference null = {0, NULL_, 0, 0};
    REQUIRE(model.frame_get(layer));
    REQUIRE(model.frame_get(null));
    auto nullCrop = model.frame_get(null)->crop;

    frames_property_set(model, {layer, null}, FrameProperty::POSITION, source);
    CHECK(model.frame_get(layer)->position == Vec2(3, 4));
    CHECK(model.frame_get(null)->position == Vec2(3, 4));

    frames_property_set(model, {layer, null}, FrameProperty::CROP, source);
    CHECK(model.frame_get(layer)->crop == Vec2(7, 8));
    CHECK(model.frame_get(null)->crop == nullCrop);

    CHECK_FALSE(is_frame_property_valid(FrameProperty::EVENT, ItemType::LAYER));
    CHECK(is_frame_property_valid(FrameProperty::SOUNDS, ItemType::TRIGGER));
    CHECK_FALSE(is_frame_property_valid(FrameProperty::POSITION, ItemType::TRIGGER));
}

TEST_CASE("frames_split splits every selected frame that spans the time")
{
    auto model = fixture_load("02_items.anm2");
    auto layerTrack = model.track_get({0, LAYER, 0});
    auto nullTrack = model.track_get({0, NULL_, 0});
    REQUIRE(layerTrack);
    REQUIRE(nullTrack);
    auto layerCount = layerTrack->frames.size();
    auto nullCount = nullTrack->frames.size();
    REQUIRE(layerTrack->frames[0].duration > 1);
    REQUIRE(nullTrack->frames[0].duration > 1);

    auto uids = frames_split(model, {{0, LAYER, 0, 0}, {0, NULL_, 0, 0}}, 0);
    CHECK(uids.size() == 2);
    CHECK(model.track_get({0, LAYER, 0})->frames.size() == layerCount + 1);
    CHECK(model.track_get({0, NULL_, 0})->frames.size() == nullCount + 1);
    CHECK(model.track_get({0, LAYER, 0})->frames[0].duration == 1);
}

TEST_CASE("change all frame properties changes layer depth only")
{
    auto model = fixture_load("02_items.anm2");
    Reference layer = {0, LAYER, 0, 0};
    Reference null = {0, NULL_, 0, 0};
    frames_change_apply(model, {.references = {layer, null}}, {.depth = 2}, ChangeType::ADD);
    frames_change_apply(model, {.references = {layer, null}}, {.depth = 1}, ChangeType::SUBTRACT);
    CHECK(model.frame_get(layer)->depth == 1);
    CHECK(model.frame_get(null)->depth == 0);
}

TEST_CASE("change all frame properties blurs layers and changes only animation layers' children")
{
    auto model = model_load(file_load(TEST_DIR / "render" / "nested.anm2"));
    Reference animationLayer = {0, LAYER, 1, 0};
    Reference spritesheetLayer = {1, LAYER, 2, 0};
    REQUIRE(model.frame_get(spritesheetLayer));
    frames_change_apply(model, {.references = {animationLayer, spritesheetLayer}},
                        {.blur = 8.0f, .childAnimation = "Spin", .childMode = ChildMode::ONCE, .childStart = 3},
                        ChangeType::ADJUST);
    auto child = *model.frame_get(animationLayer);
    CHECK(child.blur == 8.0f);
    CHECK(child.childAnimation == "Spin");
    CHECK(child.childMode == ChildMode::ONCE);
    CHECK(child.childStart == 3);
    auto sprite = *model.frame_get(spritesheetLayer);
    CHECK(sprite.blur == 8.0f);
    CHECK(sprite.childAnimation.empty());
    CHECK(sprite.childMode == ChildMode::LOOP);
    CHECK(sprite.childStart == 0);

    frames_change_apply(model, {.references = {animationLayer}}, {.blur = 1000.0f}, ChangeType::ADD);
    CHECK(model.frame_get(animationLayer)->blur == FRAME_BLUR_MAX);
}

TEST_CASE("animations_merge moves triggers past the animation placed before them")
{
    auto model_make = []() {
        auto model = fixture_load("01_minimal.anm2");
        auto first = *model.animation_get(0);
        first.frameNum = 10;
        auto second = first;
        second.uid = uid_next();
        second.triggers.frames = {Frame{.atFrame = 5}};
        model.animations.entries = {std::make_shared<const Animation>(first),
                                    std::make_shared<const Animation>(second)};
        return model;
    };

    auto appended = model_make();
    animations_merge(appended, 0, {0, 1}, MERGE_APPEND, SOURCES_KEPT, {});
    CHECK(appended.animation_get(0)->triggers.frames.at(0).atFrame == 15);

    auto prepended = model_make();
    animations_merge(prepended, 1, {0, 1}, MERGE_PREPEND, SOURCES_KEPT, {});
    CHECK(prepended.animation_get(1)->triggers.frames.at(0).atFrame == 15);
}

TEST_CASE("skew reads from the old Shear attributes too")
{
    auto frames = frames_from_string(R"(<Frame ShearX="5" ShearY="6"/><Frame SkewX="7" ShearX="9"/>)", ItemType::NULL_);
    REQUIRE(frames.size() == 2);
    CHECK(frames[0].skew == Vec2(5, 6));
    CHECK(frames[1].skew.x == 7);
    CHECK(frames[0].extras.attributes.empty());
}

TEST_CASE("a trigger fires several events; the game's copy keeps the first")
{
    auto model = fixture_load("02_items.anm2");
    model.frame_edit({0, TRIGGER, -1, 0})->eventIds = {1, 0};
    auto text = model_to_string(model);
    CHECK(model_load(text).frame_get({0, TRIGGER, -1, 0})->eventIds == std::vector{1, 0});
    CHECK(model_load(game_document_get(text)).frame_get({0, TRIGGER, -1, 0})->eventIds == std::vector{1});
    CHECK(isaac_issues_get(model).extraEvents == 1);
}

TEST_CASE("SVG spritesheets pack and merge into SVGs")
{
    auto model = model_load(file_load(TEST_DIR / "render" / "vector.anm2"));
    Image vector(TEST_DIR / "render" / "vector.svg");
    auto& regions = model.content.spritesheets.front().regions;
    regions.push_back({.id = 0, .name = "circle", .crop = {0, 0}, .size = {16, 16}});
    regions.push_back({.id = 1, .name = "square", .crop = {16, 0}, .size = {16, 16}});

    // Packing keeps the drawing: each region shows the same pixels where it now sits.
    auto packed = spritesheet_pack(model, 0, vector, 1);
    REQUIRE(packed);
    CHECK(packed->is_vector());
    for (auto [id, original] : {std::pair{0, Vec2(8, 8)}, std::pair{1, Vec2(24, 8)}})
    {
        auto region = item_get(model.content.spritesheets.front().regions, id);
        CHECK(packed->pixel_read(region->crop + Vec2(8, 8)) == vector.pixel_read(original));
    }

    // A PNG sheet merged after an SVG one: the result is an SVG holding the PNG. A PNG first takes the .svg name.
    const std::vector<uint8_t> pixels(4 * 4 * IMAGE_CHANNELS, 255);
    Image raster(pixels.data(), {4, 4});
    model.content.spritesheets.push_back({.id = 1, .path = "white.png"});
    auto images = std::map<int, const Image*>{{0, &vector}, {1, &raster}};
    auto image_get = [&](int id) {
        return images.at(id);
    };
    auto mixed = model;
    auto merged = spritesheets_merge(mixed, {0, 1}, image_get, {true, false, false, Origin::TOP_LEFT});
    REQUIRE(merged);
    CHECK(merged->is_vector());
    CHECK(merged->size == IVec2(36, 16));
    CHECK(merged->pixel_read({33, 1}) == raster.pixel_read({1, 1}));
    CHECK(mixed.content.spritesheets.front().path == "vector.svg");

    std::swap(model.content.spritesheets.front().id, model.content.spritesheets.back().id);
    images = {{1, &vector}, {0, &raster}};
    REQUIRE(spritesheets_merge(model, {0, 1}, image_get, {true, false, false, Origin::TOP_LEFT}));
    CHECK(item_get(model.content.spritesheets, 0)->path == "white.svg");
}

TEST_CASE("content_remove takes what refers to an item with it")
{
    auto path_keep = [](const Model&, const std::filesystem::path& path) {
        return path;
    };
    auto remove = [&](Model& model, ElementType type, std::set<int> ids, int spritesheetId = -1,
                      const LibraryGet& library_get = {}) {
        content_remove(model, type, ids, spritesheetId, library_get, path_keep);
    };
    auto tracks_count = [](const Model& model, int layerId) {
        int count = 0;
        for (auto [index, animation] : model.animations_get())
            items_each(animation->layers, [&](const Track& track, auto*) { count += track.id == layerId; });
        return count;
    };

    SUBCASE("a layer's tracks, and a spritesheet's plain layers")
    {
        auto model = fixture_load("02_items.anm2");
        CHECK(model.uses_count(ElementType::LAYER_ELEMENT, {0}) == tracks_count(model, 0));
        remove(model, ElementType::LAYER_ELEMENT, {0});
        CHECK_FALSE(item_get(model.content.layers, 0));
        CHECK(tracks_count(model, 0) == 0);
        CHECK(tracks_count(model, 1) > 0);

        CHECK(model.uses_count(ElementType::SPRITESHEET, {1}) == 1);
        remove(model, ElementType::SPRITESHEET, {1});
        CHECK(model.content.layers.empty());
        CHECK(tracks_count(model, 1) == 0);
        CHECK(model.content.spritesheets.size() == 1);
    }

    SUBCASE("an event or sound from its triggers, and triggers left with neither")
    {
        auto model = fixture_load("02_items.anm2");
        auto& triggers = model.animation_get(0)->triggers.frames;
        REQUIRE(triggers.size() == 3);
        CHECK(model.uses_count(ElementType::SOUND_ELEMENT, {1}) == 2);
        remove(model, ElementType::SOUND_ELEMENT, {1});
        REQUIRE(triggers.size() == 2);
        CHECK(triggers[1].soundIds == std::vector{0});
        remove(model, ElementType::EVENT_ELEMENT, {0});
        REQUIRE(triggers.size() == 2);
        CHECK(triggers[0].eventIds.empty());
        CHECK(triggers[0].soundIds == std::vector{0});
    }

    SUBCASE("a shader, and a region, from their frames")
    {
        auto shaders = fixture_load("06_shaders.anm2");
        remove(shaders, ElementType::SHADER, {0});
        CHECK(shaders.frame_get({0, LAYER, 0, 0})->shaderId == -1);
        CHECK(shaders.frame_get({0, LAYER, 0, 1})->shaderId == 4);

        auto regions = fixture_load("05_regions.anm2");
        auto before = *regions.frame_get({0, LAYER, 0, 0});
        REQUIRE(before.regionId == 0);
        CHECK(regions.uses_count(ElementType::REGION, {0}, 0) == 1);
        remove(regions, ElementType::REGION, {0}, 0);
        auto after = *regions.frame_get({0, LAYER, 0, 0});
        CHECK(after.regionId == -1);
        CHECK(after.crop == before.crop);
        CHECK(after.size == before.size);
        CHECK(after.pivot == before.pivot);
        CHECK_FALSE(item_get(regions.content.spritesheets.front().regions, 0));
    }

    SUBCASE("a library's layers, flattened")
    {
        auto model = model_load(file_load(TEST_DIR / "render" / "nested.anm2"));
        auto spin = model_load(file_load(TEST_DIR / "render" / "spin.anm2"));
        CHECK(model.uses_count(ElementType::LIBRARY, {0}) == 1);
        remove(model, ElementType::LIBRARY, {0}, -1, [&](const Model&, int id) { return id == 0 ? &spin : nullptr; });
        CHECK(model.content.libraries.empty());
        CHECK_FALSE(item_get(model.content.layers, 0));
        CHECK(std::ranges::any_of(model.content.layers, [](const Layer& layer) { return layer.name == "spin/red"; }));
        // Layers playing this file's animations stay as they are.
        CHECK(item_get(model.content.layers, 1)->is_animation());
        CHECK(item_get(model.content.layers, 3)->is_animation());
    }
}
