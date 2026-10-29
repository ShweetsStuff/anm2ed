#include "common.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

TEST_CASE("per-fixture invariants")
{
  for (const auto& fixture : fixtures_get())
  {
    INFO("fixture: ", fixture.filename().string());
    invariants_check(model_load(file_load(fixture)));
  }
}

TEST_CASE("saved text is what tinyxml2 prints for the same document")
{
  auto model = model_load(file_load(CORPUS_DIR / "11_unknown.anm2"));
  model.extras.children.emplace_back(
      0, model::XmlNode{.tag = "Note",
                        .attributes = {{"Quote", "it's \"x\" & <y>"}},
                        .text = "a & b > c 'd' \"e\"",
                        .children = {{.tag = "Inner", .children = {{.tag = "Leaf", .attributes = {{"A", "1"}}}}}}});
  auto text = model::model_to_string(model);
  tinyxml2::XMLDocument document{};
  REQUIRE(document.Parse(text.c_str()) == tinyxml2::XML_SUCCESS);
  tinyxml2::XMLPrinter printer{};
  document.Print(&printer);
  CHECK(text == printer.CStr());
}

TEST_CASE("purged default colors read back the same")
{
  for (const auto& fixture : fixtures_get())
  {
    INFO("fixture: ", fixture.string());
    auto model = model_load(file_load(fixture));
    auto text = model::model_to_string(model, {.isPurgeDefaultColors = true});
    CHECK(text.find("Tint=\"255\"") == std::string::npos);
    CHECK(text.find("Offset=\"0\"") == std::string::npos);
    CHECK(text.size() <= model::model_to_string(model).size());
    CHECK(model::model_hash(model_load(text)) == model::model_hash(model));
  }
}

TEST_CASE("JSON documents read back as the editor's copy")
{
  for (const auto& fixture : fixtures_get())
  {
    INFO("fixture: ", fixture.string());
    auto model = model_load(file_load(fixture));
    auto text = model::model_to_json(model);
    auto reloaded = model_load(text);
    CHECK(model::model_to_json(reloaded) == text);
    // Unknown XML, element order, dangling references and repeated animation names are not kept.
    auto isQuirky = fixture.filename() == "09_legacy_quirks.anm2" || fixture.filename() == "11_unknown.anm2";
    if (!isQuirky)
      CHECK(model::model_serialize(reloaded, SERIALIZE_ANM2ED_DEFAULT) ==
            model::model_serialize(model, SERIALIZE_ANM2ED_DEFAULT));
  }
}

TEST_CASE("JSON animations get unique names")
{
  auto model = model_load(file_load(CORPUS_DIR / "01_minimal.anm2"));
  model.animations.entries.push_back(model.animations.entries.front());
  auto reloaded = model_load(model::model_to_json(model));
  REQUIRE(reloaded.animations_count_get() == 2);
  CHECK(reloaded.animation_get(1)->name == reloaded.animation_get(0)->name + " 2");
}

TEST_CASE("JSON dates are ISO 8601")
{
  auto model = model_load(file_load(CORPUS_DIR / "01_minimal.anm2"));
  for (auto [date, iso] : {std::pair{"01/01/2026 12:00:00 PM", "2026-01-01T12:00:00"},
                           {"03/04/2025 12:30:05 AM", "2025-03-04T00:30:05"},
                           {"12/31/2024 11:59:59 PM", "2024-12-31T23:59:59"}})
  {
    model.info.createdOn = date;
    auto text = model::model_to_json(model);
    CHECK(text.find(iso) != std::string::npos);
    CHECK(model_load(text).info.createdOn == date);
  }
}

TEST_CASE("JSON that can't be read is reported by where it is, and the rest still opens")
{
  auto text = R"({ "anm2ed": 1, "colour": 1, "info": { "fps": "fast" },
    "layers": [ { "name": "a", "sheet": 4 } ],
    "animations": { "Idle": { "length": 3, "root": [ { "pos": [1], "interpolation": "bouncy", "delay": 3 } ] } } })";
  model::Model model{};
  std::string issues{};
  REQUIRE(model::model_load_string(model, text, &issues));
  CHECK(model.animation_get(0)->frameNum == 3);
  CHECK(model.animation_get(0)->root.frames[0].duration == 3);
  for (auto issue : {"colour: unknown key", "info.fps: expected a number", "layers[0].sheet: no item at index 4",
                     "animations.Idle.root[0].pos: expected an array of 2 numbers",
                     "animations.Idle.root[0].interpolation: unknown value \"bouncy\""})
    CHECK_MESSAGE(issues.find(issue) != std::string::npos, issue);

  CHECK_FALSE(model::model_load_string(model, "{ \"fps\": 30 }", &issues));
  CHECK_FALSE(model::model_load_string(model, "{ \"anm2ed\": 1, ", &issues));
}
