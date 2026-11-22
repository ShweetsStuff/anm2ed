#include "common.h"

TEST_CASE("per-fixture invariants")
{
    for (auto& fixture : fixtures_get())
    {
        INFO("fixture: ", fixture.filename().string());
        invariants_check(model_load(file_load(fixture)));
    }
}

TEST_CASE("saved text escapes what it holds, and reads back the same")
{
    auto model = model_load(file_load(CORPUS_DIR / "11_unknown.anm2"));
    model.extras.children.emplace_back(
        0, XmlNode{.tag = "Note",
                   .attributes = {{"Quote", "it's \"x\" & <y>"}},
                   .text = "a & b > c 'd' \"e\"",
                   .children = {{.tag = "Inner", .children = {{.tag = "Leaf", .attributes = {{"A", "1"}}}}}}});
    auto text = model_to_string(model);
    CHECK(text.find("Quote=\"it&apos;s &quot;x&quot; &amp; &lt;y&gt;\"") != std::string::npos);
    CHECK(text.find(">a &amp; b &gt; c 'd' \"e\"<") != std::string::npos);
    auto reloaded = model_load(text).extras.children;
    auto note = std::ranges::find(reloaded, std::string("Note"), [](const auto& child) { return child.second.tag; });
    REQUIRE(note != reloaded.end());
    CHECK(note->second == model.extras.children.back().second);
}

TEST_CASE("saves leave out default colors, which read back the same")
{
    for (auto& fixture : fixtures_get())
    {
        INFO("fixture: ", fixture.string());
        auto model = model_load(file_load(fixture));
        auto text = model_to_string(model);
        CHECK(text.find("Tint=\"255\"") == std::string::npos);
        CHECK(text.find("Offset=\"0\"") == std::string::npos);
        CHECK(model_hash(model_load(text)) == model_hash(model));
    }
}

TEST_CASE("JSON documents read back as the editor's copy")
{
    for (auto& fixture : fixtures_get())
    {
        INFO("fixture: ", fixture.string());
        auto model = model_load(file_load(fixture));
        auto text = model_to_json(model);
        auto reloaded = model_load(text);
        CHECK(model_to_json(reloaded) == text);
        // Unknown XML, element order, dangling references and repeated animation names are not kept.
        auto isQuirky = fixture.filename() == "09_legacy_quirks.anm2" || fixture.filename() == "11_unknown.anm2";
        if (!isQuirky)
            CHECK(model_serialize(reloaded, SERIALIZE_ANM2ED_DEFAULT) ==
                  model_serialize(model, SERIALIZE_ANM2ED_DEFAULT));
    }
}

TEST_CASE("JSON animations get unique names")
{
    auto model = model_load(file_load(CORPUS_DIR / "01_minimal.anm2"));
    model.animations.entries.push_back(model.animations.entries.front());
    auto reloaded = model_load(model_to_json(model));
    REQUIRE(reloaded.animations_count_get() == 2);
    CHECK(reloaded.animation_get(1)->name == reloaded.animation_get(0)->name + " 2");
}

TEST_CASE("JSON groups don't renumber an animation sharing their name")
{
    auto model = model_load(file_load(CORPUS_DIR / "01_minimal.anm2"));
    auto animation = std::get<std::shared_ptr<const Animation>>(model.animations.entries.front());
    model.animations.entries = {AnimationGroup{.name = animation->name, .animations = {animation}}};
    auto reloaded = model_load(model_to_json(model));
    REQUIRE(reloaded.animations_count_get() == 1);
    CHECK(reloaded.animation_get(0)->name == animation->name);
}

TEST_CASE("JSON markers are a pair, and a frame's child an object only when it has one")
{
    auto model = model_load(file_load(CORPUS_DIR / "01_minimal.anm2"));
    auto text = model_to_json(model);
    CHECK(text.find("\"child\"") == std::string::npos);
    model.animation_edit(0)->startMarker = 2;
    model.animation_edit(0)->endMarker = 5;
    auto frame = model.frame_edit({0, (int)ItemType::LAYER, 0, 0});
    REQUIRE(frame);
    frame->childMode = ChildMode::HOLD;
    frame->childStart = 3;
    text = model_to_json(model);
    CHECK(text.find("\"markers\": [2, 5]") != std::string::npos);
    CHECK(text.find("\"child\": {\"mode\": \"hold\", \"start\": 3}") != std::string::npos);
    auto reloaded = model_load(text);
    CHECK(reloaded.animation_get(0)->endMarker == 5);
    CHECK(model_to_json(reloaded) == text);
}

TEST_CASE("JSON dates are ISO 8601")
{
    auto model = model_load(file_load(CORPUS_DIR / "01_minimal.anm2"));
    for (auto [date, iso] : {std::pair{"01/01/2026 12:00:00 PM", "2026-01-01T12:00:00"},
                             {"03/04/2025 12:30:05 AM", "2025-03-04T00:30:05"},
                             {"12/31/2024 11:59:59 PM", "2024-12-31T23:59:59"}})
    {
        model.info.createdOn = date;
        auto text = model_to_json(model);
        CHECK(text.find(iso) != std::string::npos);
        CHECK(model_load(text).info.createdOn == date);
    }
}

TEST_CASE("JSON that can't be read is reported by where it is, and the rest still opens")
{
    auto text = R"({ "colour": 1, "info": { "fps": "fast" },
    "layers": [ { "name": "a", "sheet": 4 } ],
    "animations": { "Idle": { "length": 3, "root": [ { "pos": [1], "ease": "bouncy", "delay": 3 } ] } } })";
    Model model = {};
    std::string issues;
    REQUIRE(model_load_string(model, text, &issues));
    CHECK(model.animation_get(0)->frameNum == 3);
    CHECK(model.animation_get(0)->root.frames[0].duration == 3);
    for (auto issue : {"colour: unknown key", "info.fps: expected a number", "layers[0].sheet: no item at index 4",
                       "animations.Idle.root[0].pos: expected an array of 2 numbers",
                       "animations.Idle.root[0].ease: unknown value \"bouncy\""})
        CHECK_MESSAGE(issues.find(issue) != std::string::npos, issue);

    CHECK_FALSE(model_load_string(model, "{ \"fps\": 30, ", &issues));
}

TEST_CASE("JSON layers can name a library listed after them in the file")
{
    auto text = R"({ "layers": [ { "name": "shadow", "library": 0, "animation": "Small" } ],
    "libraries": [ "shared/shadows.anm2" ] })";
    Model model = {};
    std::string issues;
    REQUIRE(model_load_string(model, text, &issues));
    CHECK(issues.empty());
    CHECK(model.content.layers[0].libraryId == 0);
}
