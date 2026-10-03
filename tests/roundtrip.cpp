#include "common.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

TEST_CASE("fixtures match goldens")
{
  auto fixtures = fixtures_get();
  REQUIRE(!fixtures.empty());
  for (const auto& fixture : fixtures)
  {
    auto anm2 = anm2_load(file_load(fixture));
    for (const auto& [name, text] : variants_get(anm2))
      golden_check(GOLDEN_DIR / std::format("{}.{}.xml", fixture.stem().string(), name), text);
  }
}
