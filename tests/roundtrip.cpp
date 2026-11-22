#include "common.h"

TEST_CASE("fixtures match goldens")
{
  auto fixtures = fixtures_get();
  REQUIRE(!fixtures.empty());
  for (auto& fixture : fixtures)
  {
    auto anm2 = model_load(file_load(fixture));
    for (auto& [name, text] : variants_get(anm2))
      golden_check(GOLDEN_DIR / std::format("{}.{}.xml", fixture.stem().string(), name), text);
  }
}
