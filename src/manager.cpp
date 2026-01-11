#include "manager.h"

#include <algorithm>
#include <unordered_set>

#include <format>

#include "log.h"
#include "path_.h"
#include "sdl.h"
#include "strings.h"
#include "toast.h"
#include "vector_.h"

using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed
{
  constexpr std::size_t RECENT_LIMIT = 10;

  namespace
  {
    void ensure_parent_directory_exists(const std::filesystem::path& path)
    {
      auto parent = path.parent_path();
      if (parent.empty()) return;
      std::error_code ec{};
      std::filesystem::create_directories(parent, ec);
      if (ec) logger.warning(std::format("Could not create directory for {}: {}", path::to_utf8(path), ec.message()));
    }
  }

  void Manager::selection_history_push(int index)
  {
    if (index < 0 || index >= (int)documents.size()) return;
    selectionHistory.erase(std::remove(selectionHistory.begin(), selectionHistory.end(), index),
                           selectionHistory.end());
    selectionHistory.push_back(index);
  }

  void Manager::selection_history_cleanup(int removedIndex)
  {
    if (removedIndex >= 0)
    {
      for (auto& entry : selectionHistory)
      {
        if (entry == removedIndex)
          entry = -1;
        else if (entry > removedIndex)
          --entry;
      }
    }

    selectionHistory.erase(std::remove_if(selectionHistory.begin(), selectionHistory.end(),
                                          [&](int idx) { return idx < 0 || idx >= (int)documents.size(); }),
                           selectionHistory.end());
    if (documents.empty()) selectionHistory.clear();
  }

  std::filesystem::path Manager::recent_files_path_get() { return sdl::preferences_directory_get() / "recent.txt"; }
  std::filesystem::path Manager::autosave_path_get() { return sdl::preferences_directory_get() / "autosave.txt"; }

  Manager::Manager()
  {
    recent_files_load();
    autosave_files_load();
  }

  Document* Manager::get(int index) { return vector::find(documents, index > -1 ? index : selected); }

  Document* Manager::open(const std::filesystem::path& path, bool isNew, bool isRecent)
  {
    std::string errorString{};
    documents.emplace_back(path, isNew, &errorString);
    auto pathString = path::to_utf8(path);

    auto& document = documents.back();
    if (!document.is_valid())
    {
      documents.pop_back();
      toasts.push(
          std::vformat(localize.get(TOAST_OPEN_DOCUMENT_FAILED), std::make_format_args(pathString, errorString)));
      logger.error(std::vformat(localize.get(TOAST_OPEN_DOCUMENT_FAILED, anm2ed::ENGLISH),
                                std::make_format_args(pathString, errorString)));
      return nullptr;
    }

    if (isRecent) recent_file_add(path);

    selected = (int)documents.size() - 1;
    pendingSelected = selected;
    selection_history_push(selected);
    toasts.push(std::vformat(localize.get(TOAST_OPEN_DOCUMENT), std::make_format_args(pathString)));
    logger.info(std::vformat(localize.get(TOAST_OPEN_DOCUMENT, anm2ed::ENGLISH), std::make_format_args(pathString)));

    return &document;
  }

  void Manager::new_(const std::filesystem::path& path) { open(path, true); }

  void Manager::save(int index, const std::filesystem::path& path)
  {
    if (auto document = get(index); document)
    {
      std::string errorString{};
      ensure_parent_directory_exists(path);
      document->path = !path.empty() ? path : document->path;
      document->path.replace_extension(".anm2");
      document->save(document->path, &errorString);
      recent_file_add(document->path);
    }
  }

  void Manager::save(const std::filesystem::path& path) { save(selected, path); }

  void Manager::autosave(Document& document)
  {
    std::string errorString{};
    auto autosavePath = document.autosave_path_get();
    if (!document.autosave(&errorString)) return;

    autosaveFiles.erase(std::remove(autosaveFiles.begin(), autosaveFiles.end(), autosavePath), autosaveFiles.end());
    autosaveFiles.insert(autosaveFiles.begin(), autosavePath);

    autosave_files_write();
  }

  void Manager::close(int index)
  {
    if (!vector::in_bounds(documents, index)) return;

    const auto autosavePath = documents[index].autosave_path_get();
    autosaveFiles.erase(std::remove(autosaveFiles.begin(), autosaveFiles.end(), autosavePath), autosaveFiles.end());

    if (!autosavePath.empty())
    {
      std::error_code ec{};
      std::filesystem::remove(autosavePath, ec);
      if (ec)
        logger.warning(std::format("Could not remove autosave file {}: {}", path::to_utf8(autosavePath), ec.message()));
    }

    autosave_files_write();

    documents.erase(documents.begin() + index);
    selection_history_cleanup(index);

    if (documents.empty())
    {
      selected = -1;
      pendingSelected = -1;
      return;
    }

    if (!selectionHistory.empty())
    {
      selected = selectionHistory.back();
      selectionHistory.pop_back();
    }
    else if (selected >= index)
      selected = std::max(0, selected - 1);

    selected = std::clamp(selected, 0, (int)documents.size() - 1);
    pendingSelected = selected;

    if (selected >= 0 && selected < (int)documents.size()) documents[selected].change(Document::ALL);
  }

  void Manager::set(int index)
  {
    if (documents.empty())
    {
      selected = -1;
      pendingSelected = -1;
      return;
    }

    index = std::clamp(index, 0, (int)documents.size() - 1);
    selected = index;
    selection_history_push(selected);

    if (auto document = get()) document->change(Document::ALL);
  }

  void Manager::layer_properties_open(int id)
  {
    if (auto document = get(); document)
    {
      if (id == -1)
        editLayer = anm2::Layer();
      else
        editLayer = document->anm2.content.layers.at(id);

      document->layer.reference = id;

      layerPropertiesPopup.open();
    }
  }

  void Manager::layer_properties_trigger() { layerPropertiesPopup.trigger(); }

  void Manager::layer_properties_end() { layerPropertiesPopup.end(); }

  void Manager::layer_properties_close()
  {
    editLayer = anm2::Layer();
    layerPropertiesPopup.close();
  }

  void Manager::null_properties_open(int id)
  {
    if (auto document = get(); document)
    {
      if (id == -1)
        editNull = anm2::Null();
      else
        editNull = document->anm2.content.nulls.at(id);

      document->null.reference = id;

      nullPropertiesPopup.open();
    }
  }

  void Manager::null_properties_trigger() { nullPropertiesPopup.trigger(); }

  void Manager::null_properties_end() { nullPropertiesPopup.end(); }

  void Manager::null_properties_close()
  {
    editNull = anm2::Null();
    nullPropertiesPopup.close();
  }

  void Manager::recent_files_trim()
  {
    while (recentFiles.size() > RECENT_LIMIT)
    {
      auto oldest = std::min_element(recentFiles.begin(), recentFiles.end(),
                                     [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });
      if (oldest == recentFiles.end()) break;
      recentFiles.erase(oldest);
    }
  }

  std::vector<std::filesystem::path> Manager::recent_files_ordered() const
  {
    std::vector<std::pair<std::string, std::size_t>> orderedEntries(recentFiles.begin(), recentFiles.end());
    std::sort(orderedEntries.begin(), orderedEntries.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; });

    std::vector<std::filesystem::path> ordered;
    ordered.reserve(orderedEntries.size());
    for (const auto& [pathString, _] : orderedEntries)
      ordered.emplace_back(path::from_utf8(pathString));
    return ordered;
  }

  void Manager::recent_file_add(const std::filesystem::path& path)
  {
    if (path.empty()) return;
    std::error_code ec{};
    if (!std::filesystem::exists(path, ec))
    {
      logger.warning(std::format("Skipping missing recent file: {}", path::to_utf8(path)));
      return;
    }

    recentFiles[path::to_utf8(path)] = ++recentFilesCounter;
    recent_files_trim();
    recent_files_write();
  }

  void Manager::recent_files_load()
  {
    auto path = recent_files_path_get();

    std::ifstream file(path);
    if (!file)
    {
      logger.warning(std::format("Could not load recent files from: {}. Skipping...", path::to_utf8(path)));
      return;
    }

    logger.info(std::format("Loading recent files from: {}", path::to_utf8(path)));

    std::string line{};
    std::vector<std::string> loaded{};
    std::unordered_set<std::string> seen{};

    while (std::getline(file, line))
    {
      if (line.empty()) continue;
      if (!line.empty() && line.back() == '\r') line.pop_back();
      auto entry = path::from_utf8(line);
      std::error_code ec{};
      if (!std::filesystem::exists(entry, ec))
      {
        logger.warning(std::format("Skipping missing recent file: {}", line));
        continue;
      }
      auto entryString = path::to_utf8(entry);
      if (!seen.insert(entryString).second) continue;
      loaded.emplace_back(std::move(entryString));
    }

    recentFiles.clear();
    recentFilesCounter = 0;
    for (auto it = loaded.rbegin(); it != loaded.rend(); ++it)
    {
      recentFiles[*it] = ++recentFilesCounter;
    }
    recent_files_trim();
  }

  void Manager::recent_files_write()
  {
    auto path = recent_files_path_get();
    ensure_parent_directory_exists(path);

    std::ofstream file;
    file.open(path, std::ofstream::out | std::ofstream::trunc);

    if (!file.is_open())
    {
      logger.warning(std::format("Could not write recent files to: {}. Skipping...", path::to_utf8(path)));
      return;
    }

    auto ordered = recent_files_ordered();
    for (auto& entry : ordered)
      file << path::to_utf8(entry) << '\n';
  }

  void Manager::recent_files_clear()
  {
    recentFiles.clear();
    recentFilesCounter = 0;
    recent_files_write();
  }

  void Manager::autosave_files_open()
  {
    for (auto& path : autosaveFiles)
    {
      auto fileNamePath = path.filename();
      auto fileNameUtf8 = path::to_utf8(fileNamePath);
      if (!fileNameUtf8.empty() && fileNameUtf8.front() == '.') fileNameUtf8.erase(fileNameUtf8.begin());
      fileNamePath = path::from_utf8(fileNameUtf8);

      auto restorePath = path.parent_path() / fileNamePath;
      restorePath.replace_extension("");

      if (auto document = open(path, false, false))
      {
        document->isForceDirty = true;
        document->path = restorePath;
        document->change(Document::ALL);
      }
    }

    autosave_files_clear();
  }

  void Manager::autosave_files_load()
  {
    auto path = autosave_path_get();

    std::ifstream file(path);
    if (!file)
    {
      logger.warning(std::format("Could not load autosave files from: {}. Skipping...", path::to_utf8(path)));
      return;
    }

    logger.info(std::format("Loading autosave files from: {}", path::to_utf8(path)));

    std::string line{};

    while (std::getline(file, line))
    {
      if (line.empty()) continue;
      if (!line.empty() && line.back() == '\r') line.pop_back();
      auto entry = path::from_utf8(line);
      if (std::find(autosaveFiles.begin(), autosaveFiles.end(), entry) != autosaveFiles.end()) continue;
      autosaveFiles.emplace_back(std::move(entry));
    }
  }

  void Manager::autosave_files_write()
  {
    std::ofstream autosaveWriteFile;
    ensure_parent_directory_exists(autosave_path_get());
    autosaveWriteFile.open(autosave_path_get(), std::ofstream::out | std::ofstream::trunc);

    if (!autosaveWriteFile.is_open())
    {
      logger.warning(
          std::format("Could not write autosave files to: {}. Skipping...", path::to_utf8(autosave_path_get())));
      return;
    }

    for (auto& path : autosaveFiles)
      autosaveWriteFile << path::to_utf8(path) << "\n";
  }

  void Manager::autosave_files_clear()
  {
    for (auto& path : autosaveFiles)
    {
      std::error_code ec{};
      std::filesystem::remove(path, ec);
      if (ec) logger.warning(std::format("Could not remove autosave file {}: {}", path::to_utf8(path), ec.message()));
    }

    autosaveFiles.clear();
    autosave_files_write();
  }

  void Manager::chords_set(Settings& settings)
  {
    for (int i = 0; i < SHORTCUT_COUNT; i++)
      chords[i] = imgui::string_to_chord(settings.*SHORTCUT_MEMBERS[i]);
  }

  Manager::~Manager() { autosave_files_clear(); }
}
