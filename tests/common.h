#pragma once

#include <doctest.h>

#include <algorithm>
#include <cstdlib>
#include <format>
#include <fstream>
#include <sstream>

#include "edit/edit.h"
#include "model/json.h"
#include "model/xml.h"

inline const std::filesystem::path TEST_DIR = ANM2ED_TEST_DIR;
inline const std::filesystem::path CORPUS_DIR = TEST_DIR / "corpus";
inline const std::filesystem::path GOLDEN_DIR = TEST_DIR / "golden";

inline constexpr std::pair<const char*, SaveMode> MODES[] = {
    {"game", SAVE_GAME}, {"editor", SAVE_EDITOR}, {"clipboard", SAVE_CLIPBOARD}};
inline constexpr std::pair<const char*, Merge> MERGES[] = {{"merge_prepend", MERGE_PREPEND},
                                                           {"merge_append", MERGE_APPEND},
                                                           {"merge_replace", MERGE_REPLACE},
                                                           {"merge_ignore", MERGE_IGNORE}};

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

inline bool is_env_set(const char* name)
{
    return std::getenv(name) != nullptr;
}

inline std::vector<std::filesystem::path> fixtures_get()
{
    std::vector<std::filesystem::path> fixtures;
    for (auto& entry : std::filesystem::directory_iterator(CORPUS_DIR))
        if (entry.path().extension() == ".anm2")
            fixtures.push_back(entry.path());
    std::ranges::sort(fixtures);
    return fixtures;
}

inline Model model_load(const std::string& text)
{
    Model model = {};
    REQUIRE(model_load_string(model, text));
    return model;
}

// Every serialization and merge the editor can produce for a document. Goldens lock each one down.
inline std::vector<Variant> variants_get(const Model& model)
{
    std::vector<Variant> variants;
    for (auto [name, mode] : MODES)
        variants.emplace_back(name, model_serialize(model, mode));
    variants.emplace_back("save", model_to_string(model));
    for (auto [name, type] : MERGES)
    {
        auto merged = model;
        std::set<int> sources = {0, 1, 2};
        animations_merge(merged, 0, sources, type,
                         type == MERGE_PREPEND || type == MERGE_REPLACE ? SOURCES_DELETED : SOURCES_KEPT, {});
        variants.emplace_back(name, model_serialize(merged, SAVE_EDITOR));
    }
    return variants;
}

// The outer document of a save with its SourceDocument (the root's last child, on lines of its own) removed: what
// the game reads.
inline std::string game_document_get(std::string text)
{
    constexpr std::string_view END = "</SourceDocument>";
    auto start = text.find("\n    <SourceDocument");
    if (start != std::string::npos)
        text.erase(start, text.find(END, start) + END.size() - start);
    return text;
}

// Save/reload properties every document must hold.
inline void invariants_check(const Model& model)
{

    // The first save may repair invalid input (dangling ids, duplicates). After that, saves must be fixed points.
    auto text = model_to_string(model);
    auto reloaded = model_load(text);
    auto resaved = model_to_string(reloaded);
    CHECK(resaved == text);
    CHECK(model_hash(model_load(resaved)) == model_hash(reloaded));

    // Loading only the game's view and re-saving it for the game must not change it.
    auto game = game_document_get(model_to_string(model));
    CHECK(model_serialize(model_load(game), SAVE_GAME) == game);
}

// Compares against a stored golden. ANM2ED_UPDATE_GOLDEN (or a missing golden when isBootstrap) rewrites it.
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
