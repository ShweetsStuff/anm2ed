#pragma once

#include <doctest.h>

#include <algorithm>
#include <cstdlib>
#include <format>
#include <fstream>
#include <sstream>

#include "anm2/anm2.hpp"
#include "util/xml.hpp"

namespace anm2ed::test
{
  inline const std::filesystem::path TEST_DIR = ANM2ED_TEST_DIR;
  inline const std::filesystem::path CORPUS_DIR = TEST_DIR / "corpus";
  inline const std::filesystem::path GOLDEN_DIR = TEST_DIR / "golden";

  inline constexpr std::pair<const char*, Flags> PRESETS[] = {{"default", SERIALIZE_DEFAULT},
                                                              {"editor", SERIALIZE_EDITOR_DEFAULT},
                                                              {"isaac", SERIALIZE_ISAAC_DEFAULT},
                                                              {"anm2ed", SERIALIZE_ANM2ED_DEFAULT}};
  inline constexpr std::pair<const char*, bool> FORMATS[] = {{"save", false}, {"extended", true}};
  inline constexpr std::pair<const char*, types::merge::Type> MERGES[] = {{"merge_prepend", types::merge::PREPEND},
                                                                          {"merge_append", types::merge::APPEND},
                                                                          {"merge_replace", types::merge::REPLACE},
                                                                          {"merge_ignore", types::merge::IGNORE}};

  using Variant = std::pair<std::string, std::string>;

  inline std::string file_load(const std::filesystem::path& path)
  {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
  }

  inline void file_save(const std::filesystem::path& path, const std::string& text)
  {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
  }

  inline bool is_env_set(const char* name) { return std::getenv(name) != nullptr; }

  inline std::vector<std::filesystem::path> fixtures_get()
  {
    std::vector<std::filesystem::path> fixtures{};
    for (const auto& entry : std::filesystem::directory_iterator(CORPUS_DIR))
      if (entry.path().extension() == ".anm2") fixtures.push_back(entry.path());
    std::ranges::sort(fixtures);
    return fixtures;
  }

  inline Anm2 anm2_load(const std::string& text)
  {
    Anm2 anm2{};
    REQUIRE(anm2.load_string(text));
    return anm2;
  }

  // Every serialization and merge the editor can produce for a document; goldens lock each one down.
  inline std::vector<Variant> variants_get(const Anm2& anm2)
  {
    std::vector<Variant> variants{};
    std::string hashes{};
    for (auto [name, flags] : PRESETS)
    {
      variants.emplace_back(name, element_to_string(anm2.normalized_for_serialize(flags).root, flags));
      for (auto [formatName, isExtended] : FORMATS)
        hashes += std::format("{}.{} {:016x}\n", name, formatName, anm2.hash({flags, isExtended}));
    }
    for (auto [name, isExtended] : FORMATS)
      variants.emplace_back(name, anm2.to_string({.isExtendedFormat = isExtended}));
    for (auto [name, type] : MERGES)
    {
      auto merged = anm2;
      std::set<int> sources{0, 1, 2};
      merged.animations_merge(0, sources, type, type == types::merge::PREPEND || type == types::merge::REPLACE);
      variants.emplace_back(name, merged.to_string({.isExtendedFormat = true}));
    }
    variants.emplace_back("hashes", hashes);
    return variants;
  }

  // The outer document of a save with its SourceDocument removed: what the game reads.
  inline std::string game_document_get(const std::string& text)
  {
    tinyxml2::XMLDocument document{};
    document.Parse(text.c_str());
    if (auto source = document.RootElement()->FirstChildElement("SourceDocument"))
      document.RootElement()->DeleteChild(source);
    return util::xml::document_to_string(document);
  }

  // Save/reload properties every document must hold.
  inline void invariants_check(const Anm2& anm2)
  {

    for (auto [name, isExtended] : FORMATS)
    {
      INFO("format: ", std::string(name));
      // The first save may repair invalid input (dangling ids, duplicates); after that, saves must be fixed points.
      auto text = anm2.to_string({.isExtendedFormat = isExtended});
      auto reloaded = anm2_load(text);
      auto resaved = reloaded.to_string({.isExtendedFormat = isExtended});
      CHECK(resaved == text);
      CHECK(anm2_load(resaved).hash() == reloaded.hash());
    }

    // Loading only the game's view and re-saving it for the game must not change it.
    auto game = game_document_get(anm2.to_string());
    auto gameAnm2 = anm2_load(game);
    CHECK(element_to_string(gameAnm2.normalized_for_serialize(SERIALIZE_ISAAC_DEFAULT).root, SERIALIZE_ISAAC_DEFAULT) ==
          game);
  }

  // Compares against a stored golden; ANM2ED_UPDATE_GOLDEN (or a missing golden when isBootstrap) rewrites it.
  inline void golden_check(const std::filesystem::path& path, const std::string& text, bool isBootstrap = false)
  {
    if (is_env_set("ANM2ED_UPDATE_GOLDEN") || (isBootstrap && !std::filesystem::exists(path)))
    {
      file_save(path, text);
      return;
    }
    INFO("golden: ", path.string());
    REQUIRE(std::filesystem::exists(path));
    CHECK(file_load(path) == text);
  }
}
