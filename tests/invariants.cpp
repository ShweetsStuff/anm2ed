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
