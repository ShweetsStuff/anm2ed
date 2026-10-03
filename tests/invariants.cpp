#include "common.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

TEST_CASE("per-fixture invariants")
{
  for (const auto& fixture : fixtures_get())
  {
    INFO("fixture: ", fixture.filename().string());
    invariants_check(anm2_load(file_load(fixture)));
  }
}
