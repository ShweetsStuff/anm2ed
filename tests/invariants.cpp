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

TEST_CASE("a document's format is kept by what it saves")
{
  auto model = model_load(file_load(CORPUS_DIR / "01_minimal.anm2"));
  CHECK_FALSE(model.isExtendedFormat);
  CHECK_FALSE(model_load(model::model_to_string(model)).isExtendedFormat);

  auto rebirthHash = model::model_hash(model);
  model.isExtendedFormat = true;
  CHECK(model::model_hash(model) != rebirthHash);
  auto text = model::model_to_string(model, {.isExtendedFormat = true});
  CHECK(text.find("SourceDocument") == std::string::npos);
  auto reloaded = model_load(text);
  CHECK(reloaded.isExtendedFormat);
  CHECK(model::model_hash(reloaded) == model::model_hash(model));
}

TEST_CASE("purged default colors read back the same")
{
  for (const auto& fixture : fixtures_get())
  {
    INFO("fixture: ", fixture.string());
    auto model = model_load(file_load(fixture));
    for (auto isExtended : {false, true})
    {
      auto text = model::model_to_string(model, {.isExtendedFormat = isExtended, .isPurgeDefaultColors = true});
      CHECK(text.find("Tint=\"255\"") == std::string::npos);
      CHECK(text.find("Offset=\"0\"") == std::string::npos);
      CHECK(text.size() <= model::model_to_string(model, {.isExtendedFormat = isExtended}).size());
      auto reloaded = model_load(text);
      reloaded.isExtendedFormat = model.isExtendedFormat;
      CHECK(model::model_hash(reloaded) == model::model_hash(model));
    }
  }
}
