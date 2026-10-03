#include "common.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

// Private corpus run: ANM2ED_CORPUS=<file listing .anm2 paths>. Baselines are written on first sight to
// ANM2ED_CORPUS_BASELINE (default ~/.cache/anm2ed/corpus) and compared on every later run. Nothing here is committed.
TEST_CASE("local corpus matches baselines" * doctest::skip(!is_env_set("ANM2ED_CORPUS")))
{
  auto baseline = is_env_set("ANM2ED_CORPUS_BASELINE")
                      ? std::filesystem::path(std::getenv("ANM2ED_CORPUS_BASELINE"))
                      : std::filesystem::path(std::getenv("HOME")) / ".cache/anm2ed/corpus";
  std::istringstream list(file_load(std::getenv("ANM2ED_CORPUS")));
  std::string line{};
  while (std::getline(list, line))
  {
    if (line.empty()) continue;
    INFO("file: ", line);
    auto key = std::format("{:016x}", std::hash<std::string>{}(line));
    Anm2 anm2{};
    std::string error{};
    if (!anm2.load(line, &error))
    {
      golden_check(baseline / (key + ".error.txt"), error, true);
      continue;
    }
    for (const auto& [name, text] : variants_get(anm2))
      golden_check(baseline / std::format("{}.{}.xml", key, name), text, true);
    invariants_check(anm2);
  }
}
